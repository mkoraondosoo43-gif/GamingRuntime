#include "gaming_runtime/runtime.h"
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
    : config_(config) {}

bool Runtime::load_game(const GamePackage& package) {
    if (package.format_version != config_.supported_package_format ||
        package.id.empty() ||
        package.name.empty() ||
        package.version.empty() ||
        package.entry_point.empty() ||
        package.root_directory.empty() ||
        package.assets_directory.empty()) {
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

    game_ = package;
    frame_ = {};
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

    if (assets_relative.empty()) {
        return false;
    }

    const std::filesystem::path assets_path = package_root / assets_relative;

    GamePackage package{
        .format_version = static_cast<std::uint32_t>(
            json_uint64(manifest, "format_version")),
        .id = json_string(manifest, "id"),
        .name = json_string(manifest, "name"),
        .version = json_string(manifest, "version"),
        .entry_point = json_string(manifest, "entry_point"),
        .root_directory = package_root.lexically_normal().string(),
        .assets_directory = assets_path.lexically_normal().string(),
        .estimated_memory_mb = json_uint64(manifest, "estimated_memory_mb")
    };

    return load_game(package);
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
}

const FrameState& Runtime::frame_state() const noexcept {
    return frame_;
}

const GamePackage& Runtime::loaded_game() const noexcept {
    return game_;
}

} // namespace gaming_runtime
