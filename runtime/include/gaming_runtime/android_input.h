#pragma once

#include "gaming_runtime/input.h"

#include <cstddef>
#include <cstdint>

struct AInputEvent;

namespace gaming_runtime {

class AndroidInputBridge final {
public:
    explicit AndroidInputBridge(InputManager& input);

    AndroidInputBridge(const AndroidInputBridge&) = delete;
    AndroidInputBridge& operator=(const AndroidInputBridge&) = delete;

    bool map_key(std::int32_t key_code, std::size_t button);
    bool map_axis(std::int32_t axis, std::size_t runtime_axis);
    void clear_mappings();

    bool handle_event(const AInputEvent* event);

private:
    InputManager* input_ = nullptr;
    std::int16_t key_buttons_[256]{};
    std::int16_t axis_slots_[64]{};
};

} // namespace gaming_runtime
