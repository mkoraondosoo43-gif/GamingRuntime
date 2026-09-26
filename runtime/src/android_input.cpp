#include "gaming_runtime/android_input.h"

#include <android/input.h>

#include <algorithm>

namespace gaming_runtime {

AndroidInputBridge::AndroidInputBridge(InputManager& input)
    : input_(&input) {
    std::fill(std::begin(key_buttons_), std::end(key_buttons_), -1);
    std::fill(std::begin(axis_slots_), std::end(axis_slots_), -1);
}

bool AndroidInputBridge::map_key(std::int32_t key_code, std::size_t button) {
    if (key_code < 0 ||
        static_cast<std::size_t>(key_code) >= std::size(key_buttons_) ||
        button >= InputState::max_buttons) {
        return false;
    }

    key_buttons_[static_cast<std::size_t>(key_code)] =
        static_cast<std::int16_t>(button);
    return true;
}

bool AndroidInputBridge::map_axis(std::int32_t axis, std::size_t runtime_axis) {
    if (axis < 0 ||
        static_cast<std::size_t>(axis) >= std::size(axis_slots_) ||
        runtime_axis >= InputState::max_axes) {
        return false;
    }

    axis_slots_[static_cast<std::size_t>(axis)] =
        static_cast<std::int16_t>(runtime_axis);
    return true;
}

void AndroidInputBridge::clear_mappings() {
    std::fill(std::begin(key_buttons_), std::end(key_buttons_), -1);
    std::fill(std::begin(axis_slots_), std::end(axis_slots_), -1);
}

bool AndroidInputBridge::handle_event(const AInputEvent* event) {
    if (!input_ || !event) {
        return false;
    }

    const int32_t type = AInputEvent_getType(event);

    if (type == AINPUT_EVENT_TYPE_KEY) {
        const int32_t key_code = AKeyEvent_getKeyCode(event);
        if (key_code < 0 ||
            static_cast<std::size_t>(key_code) >= std::size(key_buttons_)) {
            return false;
        }

        const std::int16_t button =
            key_buttons_[static_cast<std::size_t>(key_code)];
        if (button < 0) {
            return false;
        }

        const int32_t action = AKeyEvent_getAction(event);
        if (action == AKEY_EVENT_ACTION_DOWN) {
            input_->set_button(static_cast<std::size_t>(button), true);
            return true;
        }

        if (action == AKEY_EVENT_ACTION_UP) {
            input_->set_button(static_cast<std::size_t>(button), false);
            return true;
        }

        return false;
    }

    if (type != AINPUT_EVENT_TYPE_MOTION) {
        return false;
    }

    const int32_t action =
        AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;

    if (action == AMOTION_EVENT_ACTION_DOWN ||
        action == AMOTION_EVENT_ACTION_MOVE) {
        const std::size_t pointer_index =
            static_cast<std::size_t>(
                (AMotionEvent_getAction(event) &
                 AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >>
                AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT);

        if (pointer_index >=
            static_cast<std::size_t>(AMotionEvent_getPointerCount(event))) {
            return false;
        }

        input_->set_pointer(
            AMotionEvent_getX(event, pointer_index),
            AMotionEvent_getY(event, pointer_index),
            true);
    } else if (action == AMOTION_EVENT_ACTION_UP ||
               action == AMOTION_EVENT_ACTION_CANCEL) {
        input_->set_pointer(
            input_->state().pointer_x,
            input_->state().pointer_y,
            false);
    } else {
        return false;
    }

    for (std::size_t axis = 0; axis < std::size(axis_slots_); ++axis) {
        const std::int16_t runtime_axis = axis_slots_[axis];
        if (runtime_axis < 0) {
            continue;
        }

        const float value = AMotionEvent_getAxisValue(
            event,
            static_cast<int32_t>(axis),
            0);
        input_->set_axis(
            static_cast<std::size_t>(runtime_axis),
            value);
    }

    return true;
}

} // namespace gaming_runtime
