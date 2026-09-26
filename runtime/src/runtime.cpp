#include "gaming_runtime/runtime.h"

#include <algorithm>

namespace gaming_runtime {

Runtime::Runtime(RuntimeConfig config)
    : config_(config) {}

bool Runtime::load_game(const GamePackage& package) {
    if (package.id.empty() ||
        package.name.empty() ||
        package.entry_point.empty()) {
        return false;
    }

    game_ = package;
    frame_ = {};
    game_loaded_ = true;
    return true;
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
