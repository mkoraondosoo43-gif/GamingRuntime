#pragma once

#include <cstdint>

namespace gaming_runtime {

class FrameScheduler {
public:
    explicit FrameScheduler(std::uint32_t target_fps = 60);

    double clamp_delta(double delta_seconds) const noexcept;
    double frame_budget_seconds() const noexcept;
    bool is_over_budget(double delta_seconds) const noexcept;
    std::uint32_t target_fps() const noexcept;

private:
    std::uint32_t target_fps_ = 60;
    double frame_budget_seconds_ = 1.0 / 60.0;
};

} // namespace gaming_runtime
