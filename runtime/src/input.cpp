#include "gaming_runtime/input.h"

#include <algorithm>

namespace gaming_runtime {

void InputManager::clear() {
    state_ = {};
}

void InputManager::set_button(std::size_t button, bool down) {
    if (button >= InputState::max_buttons) {
        return;
    }
    state_.buttons[button] = down;
}

void InputManager::set_axis(std::size_t axis, float value) {
    if (axis >= InputState::max_axes) {
        return;
    }
    state_.axes[axis] = std::clamp(value, -1.0f, 1.0f);
}

void InputManager::set_pointer(float x, float y, bool down) {
    state_.pointer_x = x;
    state_.pointer_y = y;
    state_.pointer_down = down;
}

const InputState& InputManager::state() const noexcept {
    return state_;
}

} // namespace gaming_runtime
