#include "gaming_runtime/frame_scheduler.h"

#include <algorithm>
#include <cmath>

namespace gaming_runtime {

FrameScheduler::FrameScheduler(std::uint32_t target_fps)
    : target_fps_(target_fps == 0 ? 60 : target_fps),
      frame_budget_seconds_(1.0 / static_cast<double>(target_fps_)) {}

double FrameScheduler::clamp_delta(double delta_seconds) const noexcept {
    if (!std::isfinite(delta_seconds)) {
        return 0.0;
    }

    return std::clamp(delta_seconds, 0.0, 0.25);
}

double FrameScheduler::frame_budget_seconds() const noexcept {
    return frame_budget_seconds_;
}

bool FrameScheduler::is_over_budget(double delta_seconds) const noexcept {
    return clamp_delta(delta_seconds) > frame_budget_seconds_;
}

std::uint32_t FrameScheduler::target_fps() const noexcept {
    return target_fps_;
}

} // namespace gaming_runtime
