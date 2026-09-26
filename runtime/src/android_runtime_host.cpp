#include "gaming_runtime/android_runtime_host.h"

#include "gaming_runtime/android_input.h"
#include "gaming_runtime/android_surface_display.h"
#include "gaming_runtime/runtime.h"

#include <android/native_window.h>

#include <limits>
#include <utility>

namespace gaming_runtime {

AndroidRuntimeHost::AndroidRuntimeHost(Runtime& runtime)
    : runtime_(&runtime),
      input_bridge_(std::make_unique<AndroidInputBridge>(runtime.input_manager())) {}

AndroidRuntimeHost::~AndroidRuntimeHost() {
    detach_surface();
}

bool AndroidRuntimeHost::attach_surface(ANativeWindow* window) {
    if (!runtime_ || !window || runtime_->game_started()) {
        return false;
    }

    detach_surface();

    const int32_t native_width = ANativeWindow_getWidth(window);
    const int32_t native_height = ANativeWindow_getHeight(window);
    if (native_width <= 0 || native_height <= 0) {
        return false;
    }

    auto display = std::make_unique<AndroidSurfaceDisplay>();
    if (!display->set_window(window)) {
        return false;
    }

    const auto width = static_cast<std::uint32_t>(native_width);
    const auto height = static_cast<std::uint32_t>(native_height);

    if (!runtime_->attach_display(std::move(display), width, height)) {
        return false;
    }

    surface_width_ = width;
    surface_height_ = height;
    surface_attached_ = true;
    return true;
}

bool AndroidRuntimeHost::resize_surface() {
    if (!runtime_ || !surface_attached_) {
        return false;
    }

    return runtime_->resize_display(surface_width_, surface_height_);
}

void AndroidRuntimeHost::detach_surface() noexcept {
    if (runtime_) {
        runtime_->detach_display();
    }

    surface_attached_ = false;
    surface_width_ = 0;
    surface_height_ = 0;
}

bool AndroidRuntimeHost::map_key(std::int32_t key_code, std::size_t button) {
    return input_bridge_ && input_bridge_->map_key(key_code, button);
}

bool AndroidRuntimeHost::map_axis(
    std::int32_t axis,
    std::size_t runtime_axis) {
    return input_bridge_ && input_bridge_->map_axis(axis, runtime_axis);
}

void AndroidRuntimeHost::clear_input_mappings() {
    if (input_bridge_) {
        input_bridge_->clear_mappings();
    }
}

bool AndroidRuntimeHost::handle_input_event(const AInputEvent* event) {
    return input_bridge_ && input_bridge_->handle_event(event);
}

bool AndroidRuntimeHost::start() {
    return runtime_ && surface_attached_ && runtime_->start_game();
}

void AndroidRuntimeHost::stop() noexcept {
    if (runtime_) {
        runtime_->stop_game();
    }
}

void AndroidRuntimeHost::tick(double delta_seconds) {
    if (runtime_) {
        runtime_->tick(delta_seconds);
    }
}

bool AndroidRuntimeHost::surface_attached() const noexcept {
    return surface_attached_;
}

std::uint32_t AndroidRuntimeHost::surface_width() const noexcept {
    return surface_width_;
}

std::uint32_t AndroidRuntimeHost::surface_height() const noexcept {
    return surface_height_;
}

} // namespace gaming_runtime
