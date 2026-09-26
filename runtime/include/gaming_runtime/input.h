#pragma once

#include <cstdint>

namespace gaming_runtime {

struct InputState {
    static constexpr std::size_t max_buttons = 64;
    static constexpr std::size_t max_axes = 8;

    bool button_down(std::size_t button) const noexcept {
        return button < max_buttons && buttons[button];
    }

    float axis_value(std::size_t axis) const noexcept {
        return axis < max_axes ? axes[axis] : 0.0f;
    }

    bool buttons[max_buttons]{};
    float axes[max_axes]{};
    float pointer_x = 0.0f;
    float pointer_y = 0.0f;
    bool pointer_down = false;
};

class InputManager {
public:
    InputManager() = default;

    void reset_frame();
    void set_button(std::size_t button, bool down);
    void set_axis(std::size_t axis, float value);
    void set_pointer(float x, float y, bool down);

    const InputState& state() const noexcept;

private:
    InputState state_{};
};

} // namespace gaming_runtime
