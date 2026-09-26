#include "gaming_runtime/runtime.h"
#include "gaming_runtime/bytecode.h"
#include "gaming_runtime/storage.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>

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
    : config_(config), render_frame_(4096) {}

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
    const auto relative_bytecode = std::filesystem::relative(
        root.lexically_normal(), bytecode.lexically_normal(), error);
    if (error) {
        return false;
    }
    const std::string relative_bytecode_text = relative_bytecode.generic_string();
    if (relative_bytecode_text == ".." ||
        relative_bytecode_text.rfind("../", 0) == 0) {
        return false;
    }

    if (!asset_manager_.set_root(package.assets_directory)) {
        return false;
    }

    game_ = package;
    frame_ = {};
    game_started_ = false;
    game_module_.reset();
    render_frame_.reset();
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

    GamePackage package{
        .format_version = static_cast<std::uint32_t>(
            json_uint64(manifest, "format_version")),
        .id = json_string(manifest, "id"),
        .name = json_string(manifest, "name"),
        .version = json_string(manifest, "version"),
        .entry_point = json_string(manifest, "entry_point"),
        .root_directory = package_root.lexically_normal().string(),
        .assets_directory = assets_path.lexically_normal().string(),
        .bytecode_path = bytecode_path.string(),
        .estimated_memory_mb = json_uint64(manifest, "estimated_memory_mb")
    };

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
    const auto relative = std::filesystem::relative(
        root.lexically_normal(), bytecode.lexically_normal(), error);
    if (error || relative.generic_string() == ".." ||
        relative.generic_string().rfind("../", 0) == 0 ||
        !std::filesystem::is_regular_file(bytecode, error) || error) {
        return false;
    }

    auto module = std::make_unique<BytecodeGameModule>(bytecode_path);
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

    const double clamped = std::clamp(delta_seconds, 0.0, 0.25);

    frame_.delta_seconds = clamped;
    ++frame_.frame_number;

    render_frame_.reset();

    if (game_started_ && game_module_) {
        game_module_->update({
            .frame_number = frame_.frame_number,
            .delta_seconds = frame_.delta_seconds
        });

        if (renderer_ && renderer_started_) {
            renderer_->submit(render_frame_);
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

    if (renderer_ && renderer_started_) {
        renderer_->shutdown();
    }

    renderer_ = std::move(renderer);
    renderer_started_ = renderer_->initialize(width, height);
    if (!renderer_started_) {
        renderer_.reset();
    }
    return renderer_started_;
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

    Texture texture{
        .width = width,
        .height = height,
        .pixels = std::move(data)
    };

    return renderer_->upload_texture(resource_id, texture);
}

} // namespace gaming_runtime
