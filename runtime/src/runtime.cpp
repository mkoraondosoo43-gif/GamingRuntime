#include "gaming_runtime/runtime.h"
#include "gaming_runtime/bytecode.h"
#include "gaming_runtime/storage.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>
#include <vector>

namespace gaming_runtime {
namespace {

std::string read_file(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        return {};
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::string json_string(const std::string& text, const std::string& key) {
    const std::regex pattern(
        "\"" + key + "\"\\s*:\\s*\"([^\"]*)\"");

    std::smatch match;
    if (!std::regex_search(text, match, pattern)) {
        return {};
    }

    return match[1].str();
}

std::uint64_t json_uint64(const std::string& text, const std::string& key) {
    const std::regex pattern(
        "\"" + key + "\"\\s*:\\s*([0-9]+)");

    std::smatch match;
    if (!std::regex_search(text, match, pattern)) {
        return 0;
    }

    try {
        return std::stoull(match[1].str());
    } catch (...) {
        return 0;
    }
}

} // namespace

Runtime::Runtime(RuntimeConfig config)
    : config_(config),
      render_frame_(4096),
      memory_manager_(config.max_memory_mb),
      frame_scheduler_(config.target_fps) {}

bool Runtime::load_game(const GamePackage& package) {
    if (package.format_version != config_.supported_package_format ||
        package.id.empty() ||
        package.name.empty() ||
        package.version.empty() ||
        package.entry_point.empty() ||
        package.root_directory.empty() ||
        package.assets_directory.empty() ||
        package.bytecode_path.empty()) {
        return false;
    }

    std::error_code error;
    const std::filesystem::path root(package.root_directory);
    const std::filesystem::path assets(package.assets_directory);

    if (!std::filesystem::is_directory(root, error) ||
        error ||
        !std::filesystem::is_directory(assets, error) ||
        error) {
        return false;
    }

    if (!std::filesystem::is_regular_file(package.bytecode_path, error) || error) {
        return false;
    }

    const std::filesystem::path bytecode(package.bytecode_path);
    const auto canonical_root =
        std::filesystem::weakly_canonical(root, error);
    if (error) {
        return false;
    }

    error.clear();
    const auto canonical_bytecode =
        std::filesystem::weakly_canonical(bytecode, error);
    if (error) {
        return false;
    }

    const auto relative_bytecode = std::filesystem::relative(
        canonical_root, canonical_bytecode, error);
    if (error) {
        return false;
    }

    const std::string relative_bytecode_text =
        relative_bytecode.generic_string();
    if (relative_bytecode_text == ".." ||
        relative_bytecode_text.rfind("../", 0) == 0) {
        return false;
    }

    if (!asset_manager_.set_root(package.assets_directory)) {
        return false;
    }

    const std::uint64_t estimated_bytes =
        package.estimated_memory_mb > (UINT64_MAX / (1024ULL * 1024ULL))
            ? UINT64_MAX
            : package.estimated_memory_mb * 1024ULL * 1024ULL;
    if (estimated_bytes > memory_manager_.budget_bytes()) {
        return false;
    }

    memory_manager_.clear();
    texture_memory_.clear();
    if (!memory_manager_.reserve(estimated_bytes)) {
        return false;
    }

    game_ = package;
    frame_ = {};
    game_started_ = false;
    game_module_.reset();
    render_frame_.reset();
    audio_frame_.reset();
    if (renderer_ && renderer_started_) {
        renderer_->shutdown();
    }
    renderer_.reset();
    renderer_memory_bytes_ = 0;
    renderer_started_ = false;
    if (audio_ && audio_started_) {
        audio_->shutdown();
    }
    audio_.reset();
    audio_memory_bytes_ = 0;
    audio_started_ = false;
    if (display_ && display_started_) {
        display_->shutdown();
    }
    display_.reset();
    display_memory_bytes_ = 0;
    display_started_ = false;
    input_manager_.clear();
    game_loaded_ = true;
    return true;
}

bool Runtime::load_manifest(const std::string& manifest_path) {
    const std::string manifest = read_file(manifest_path);
    if (manifest.empty()) {
        return false;
    }

    const std::filesystem::path manifest_file(manifest_path);
    const std::filesystem::path package_root = manifest_file.parent_path();
    const std::string assets_relative = json_string(manifest, "assets");
    const std::string bytecode_relative = json_string(manifest, "bytecode");

    if (assets_relative.empty() || bytecode_relative.empty()) {
        return false;
    }

    const std::filesystem::path assets_path =
        (package_root / assets_relative).lexically_normal();

    std::error_code path_error;
    const std::filesystem::path relative_assets =
        std::filesystem::relative(
            package_root.lexically_normal(),
            assets_path,
            path_error);

    if (path_error) {
        return false;
    }

    const std::string relative_text = relative_assets.generic_string();
    if (relative_text == ".." ||
        relative_text.rfind("../", 0) == 0) {
        return false;
    }

    const std::filesystem::path bytecode_path =
        (package_root / bytecode_relative).lexically_normal();
    const auto relative_bytecode = std::filesystem::relative(
        package_root.lexically_normal(), bytecode_path, path_error);
    if (path_error) {
        return false;
    }
    const std::string relative_bytecode_text = relative_bytecode.generic_string();
    if (relative_bytecode_text == ".." ||
        relative_bytecode_text.rfind("../", 0) == 0) {
        return false;
    }

    GamePackage package;
    package.format_version = static_cast<std::uint32_t>(
        json_uint64(manifest, "format_version"));
    package.id = json_string(manifest, "id");
    package.name = json_string(manifest, "name");
    package.version = json_string(manifest, "version");
    package.entry_point = json_string(manifest, "entry_point");
    package.root_directory = package_root.lexically_normal().string();
    package.assets_directory = assets_path.lexically_normal().string();
    package.bytecode_path = bytecode_path.string();
    package.estimated_memory_mb = json_uint64(manifest, "estimated_memory_mb");

    if (!load_game(package)) {
        return false;
    }

    return load_bytecode_module(package.bytecode_path);
}

bool Runtime::load_game_from_storage(
    const std::string& game_id,
    const std::string& storage_root) {

    if (game_id.empty() || storage_root.empty()) {
        return false;
    }

    GameStorage storage(storage_root);
    const auto games = storage.discover_games();

    for (const auto& stored : games) {
        const std::string manifest = read_file(stored.manifest_path);

        if (manifest.empty()) {
            continue;
        }

        if (json_string(manifest, "id") == game_id) {
            return load_manifest(stored.manifest_path);
        }
    }

    return false;
}

bool Runtime::load_bytecode_module(const std::string& bytecode_path) {
    if (!game_loaded_ || game_started_ || bytecode_path.empty()) {
        return false;
    }

    std::error_code error;
    const std::filesystem::path root(game_.root_directory);
    const std::filesystem::path bytecode(bytecode_path);
    const auto canonical_root =
        std::filesystem::weakly_canonical(root, error);
    if (error) {
        return false;
    }

    error.clear();
    const auto canonical_bytecode =
        std::filesystem::weakly_canonical(bytecode, error);
    if (error) {
        return false;
    }

    const auto relative =
        std::filesystem::relative(canonical_root, canonical_bytecode, error);
    if (error || relative.generic_string() == ".." ||
        relative.generic_string().rfind("../", 0) == 0 ||
        !std::filesystem::is_regular_file(canonical_bytecode, error) || error) {
        return false;
    }

    auto module =
        std::make_unique<BytecodeGameModule>(canonical_bytecode.string());
    game_module_ = std::move(module);
    return true;
}

bool Runtime::attach_game_module(std::unique_ptr<GameModule> module) {
    if (!game_loaded_ || game_started_ || !module) {
        return false;
    }

    game_module_ = std::move(module);
    return true;
}

bool Runtime::start_game() {
    if (!game_loaded_ || !can_run_locally() ||
        game_started_ || !game_module_) {
        return false;
    }

    if (!game_module_->initialize()) {
        game_module_.reset();
        return false;
    }

    game_started_ = true;
    return true;
}

void Runtime::stop_game() {
    if (!game_started_) {
        return;
    }

    if (game_module_) {
        game_module_->shutdown();
    }

    render_frame_.reset();
    audio_frame_.reset();
    game_started_ = false;
}

bool Runtime::can_run_locally() const {
    if (!game_loaded_) {
        return false;
    }

    return game_.estimated_memory_mb <= config_.max_memory_mb;
}

void Runtime::tick(double delta_seconds) {
    if (!game_loaded_) {
        return;
    }

    const double clamped = frame_scheduler_.clamp_delta(delta_seconds);

    frame_.delta_seconds = clamped;
    frame_.frame_budget_seconds = frame_scheduler_.frame_budget_seconds();
    frame_.frame_over_budget = frame_scheduler_.is_over_budget(delta_seconds);
    ++frame_.frame_number;

    render_frame_.reset();
    audio_frame_.reset();

    if (game_started_ && game_module_) {
        GameFrameContext context;
        context.frame_number = frame_.frame_number;
        context.delta_seconds = frame_.delta_seconds;
        context.input = &input_manager_.state();
        game_module_->update(context);

        if (renderer_ && renderer_started_) {
            renderer_->submit(render_frame_);
        }

        if (audio_ && audio_started_) {
            audio_->submit(audio_frame_);
        }

        if (display_ && display_started_) {
            display_->present();
        }
    }
}

const FrameState& Runtime::frame_state() const noexcept {
    return frame_;
}

const GamePackage& Runtime::loaded_game() const noexcept {
    return game_;
}

bool Runtime::game_started() const noexcept {
    return game_started_;
}

const AssetManager& Runtime::assets() const noexcept {
    return asset_manager_;
}

RenderFrame& Runtime::render_frame() noexcept {
    return render_frame_;
}

const RenderFrame& Runtime::render_frame() const noexcept {
    return render_frame_;
}

bool Runtime::attach_renderer(std::unique_ptr<Renderer> renderer,
                                  std::uint32_t width,
                                  std::uint32_t height) {
    if (!game_loaded_ || game_started_ || !renderer ||
        width == 0 || height == 0) {
        return false;
    }

    if (!renderer->initialize(width, height)) {
        return false;
    }

    const std::uint64_t new_bytes = renderer->memory_bytes();
    const std::uint64_t old_bytes =
        renderer_started_ ? renderer_memory_bytes_ : 0;

    const std::uint64_t available_bytes = memory_manager_.available_bytes();
    if (new_bytes > available_bytes &&
        new_bytes - available_bytes > old_bytes) {
        renderer->shutdown();
        return false;
    }

    if (renderer_ && renderer_started_) {
        renderer_->shutdown();
        memory_manager_.release(old_bytes);
    }

    renderer_ = std::move(renderer);
    renderer_memory_bytes_ = new_bytes;
    renderer_started_ = true;

    if (!memory_manager_.reserve(new_bytes)) {
        renderer_->shutdown();
        renderer_.reset();
        renderer_memory_bytes_ = 0;
        renderer_started_ = false;
        return false;
    }

    return true;
}

AudioFrame& Runtime::audio_frame() noexcept {
    return audio_frame_;
}

const AudioFrame& Runtime::audio_frame() const noexcept {
    return audio_frame_;
}

bool Runtime::attach_audio(std::unique_ptr<AudioBackend> audio,
                           std::uint32_t sample_rate,
                           std::uint32_t channels) {
    if (!game_loaded_ || game_started_ || !audio ||
        sample_rate == 0 || channels == 0) {
        return false;
    }

    if (!audio->initialize(sample_rate, channels)) {
        return false;
    }

    const std::uint64_t new_bytes = audio->memory_bytes();
    const std::uint64_t old_bytes =
        audio_started_ ? audio_memory_bytes_ : 0;

    const std::uint64_t available_bytes = memory_manager_.available_bytes();
    if (new_bytes > available_bytes &&
        new_bytes - available_bytes > old_bytes) {
        audio->shutdown();
        return false;
    }

    if (audio_ && audio_started_) {
        audio_->shutdown();
        memory_manager_.release(old_bytes);
    }

    audio_ = std::move(audio);
    audio_memory_bytes_ = new_bytes;
    audio_started_ = true;

    if (!memory_manager_.reserve(new_bytes)) {
        audio_->shutdown();
        audio_.reset();
        audio_memory_bytes_ = 0;
        audio_started_ = false;
        return false;
    }

    return true;
}

bool Runtime::attach_display(std::unique_ptr<DisplayBackend> display,
                              std::uint32_t width,
                              std::uint32_t height) {
    if (!game_loaded_ || game_started_ || !display ||
        width == 0 || height == 0) {
        return false;
    }

    if (!display->initialize(width, height)) {
        return false;
    }

    const std::uint64_t new_bytes = display->memory_bytes();
    const std::uint64_t old_bytes =
        display_started_ ? display_memory_bytes_ : 0;

    const std::uint64_t available_bytes = memory_manager_.available_bytes();
    if (new_bytes > available_bytes &&
        new_bytes - available_bytes > old_bytes) {
        display->shutdown();
        return false;
    }

    if (display_ && display_started_) {
        display_->shutdown();
        memory_manager_.release(old_bytes);
    }

    display_ = std::move(display);
    display_memory_bytes_ = new_bytes;
    display_started_ = true;

    if (!memory_manager_.reserve(new_bytes)) {
        display_->shutdown();
        display_.reset();
        display_memory_bytes_ = 0;
        display_started_ = false;
        return false;
    }

    return true;
}

bool Runtime::load_texture_asset(std::uint32_t resource_id,
                                 const std::string& relative_path,
                                 std::uint32_t width,
                                 std::uint32_t height) {
    if (!game_loaded_ || !renderer_started_ || !renderer_ ||
        resource_id == 0 || width == 0 || height == 0) {
        return false;
    }

    constexpr std::uint64_t max_pixels = 16ULL * 1024ULL * 1024ULL;
    const std::uint64_t pixel_count =
        static_cast<std::uint64_t>(width) * height;
    if (pixel_count > max_pixels || pixel_count > (UINT64_MAX / 4ULL)) {
        return false;
    }

    const std::uint64_t expected_bytes = pixel_count * 4ULL;
    std::vector<std::uint8_t> data;
    if (!asset_manager_.read_asset(relative_path, data, expected_bytes) ||
        data.size() != expected_bytes) {
        return false;
    }

    const auto existing = texture_memory_.find(resource_id);
    const std::uint64_t previous_bytes =
        existing == texture_memory_.end() ? 0 : existing->second;

    if (expected_bytes > previous_bytes) {
        if (!memory_manager_.reserve(expected_bytes - previous_bytes)) {
            return false;
        }
    }

    Texture texture{width, height, std::move(data)};
    if (!renderer_->upload_texture(resource_id, texture)) {
        if (expected_bytes > previous_bytes) {
            memory_manager_.release(expected_bytes - previous_bytes);
        }
        return false;
    }

    if (expected_bytes < previous_bytes) {
        memory_manager_.release(previous_bytes - expected_bytes);
    }

    texture_memory_[resource_id] = expected_bytes;
    return true;
}

bool Runtime::unload_texture(std::uint32_t resource_id) {
    if (!game_loaded_ || !renderer_started_ || !renderer_ ||
        resource_id == 0) {
        return false;
    }

    const auto it = texture_memory_.find(resource_id);
    if (it == texture_memory_.end()) {
        return false;
    }

    const bool removed = renderer_->unregister_texture(resource_id);
    if (!removed) {
        return false;
    }

    memory_manager_.release(it->second);
    texture_memory_.erase(it);
    return true;
}

} // namespace gaming_runtime

void gaming_runtime::Runtime::set_input_button(std::size_t button, bool down) {
    input_manager_.set_button(button, down);
}

void gaming_runtime::Runtime::set_input_axis(std::size_t axis, float value) {
    input_manager_.set_axis(axis, value);
}

void gaming_runtime::Runtime::set_pointer_input(float x, float y, bool down) {
    input_manager_.set_pointer(x, y, down);
}

const gaming_runtime::InputState& gaming_runtime::Runtime::input_state() const noexcept {
    return input_manager_.state();
}

const gaming_runtime::MemoryManager& gaming_runtime::Runtime::memory() const noexcept {
    return memory_manager_;
}

const gaming_runtime::FrameScheduler& gaming_runtime::Runtime::frame_scheduler() const noexcept {
    return frame_scheduler_;
}
