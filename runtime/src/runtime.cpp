#include "gaming_runtime/runtime.h"

#include <algorithm>
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
        R"(")" + key + R"("s*:s*"([^"]*)")");

    std::smatch match;
    if (!std::regex_search(text, match, pattern)) {
        return {};
    }

    return match[1].str();
}

std::uint64_t json_uint64(const std::string& text, const std::string& key) {
    const std::regex pattern(
        R"(")" + key + R"("s*:s*([0-9]+))");

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
    if (package.id.empty() ||
        package.name.empty() ||
        package.version.empty() ||
        package.entry_point.empty()) {
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

    GamePackage package{
        .id = json_string(manifest, "id"),
        .name = json_string(manifest, "name"),
        .version = json_string(manifest, "version"),
        .entry_point = json_string(manifest, "entry_point"),
        .estimated_memory_mb = json_uint64(manifest, "estimated_memory_mb")
    };

    return load_game(package);
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
