#include "gaming_runtime/android_surface_display.h"

#include <android/native_window.h>

namespace gaming_runtime {

AndroidSurfaceDisplay::~AndroidSurfaceDisplay() {
    shutdown();
}

bool AndroidSurfaceDisplay::set_window(ANativeWindow* window) {
    if (initialized_) {
        return false;
    }

    if (window_ == window) {
        return window != nullptr;
    }

    if (window_) {
        ANativeWindow_release(window_);
        window_ = nullptr;
    }

    if (!window) {
        return false;
    }

    ANativeWindow_acquire(window);
    window_ = window;
    return true;
}

void AndroidSurfaceDisplay::clear_window() {
    if (initialized_) {
        shutdown();
    }

    if (window_) {
        ANativeWindow_release(window_);
        window_ = nullptr;
    }
}

bool AndroidSurfaceDisplay::initialize(std::uint32_t width, std::uint32_t height) {
    if (!window_ || width == 0 || height == 0) {
        return false;
    }

    if (ANativeWindow_setBuffersGeometry(
            window_,
            static_cast<int32_t>(width),
            static_cast<int32_t>(height),
            WINDOW_FORMAT_RGBA_8888) != 0) {
        return false;
    }

    width_ = width;
    height_ = height;
    initialized_ = true;
    return true;
}

bool AndroidSurfaceDisplay::resize(std::uint32_t width, std::uint32_t height) {
    if (!initialized_ || !window_ || width == 0 || height == 0) {
        return false;
    }

    return initialize(width, height);
}

bool AndroidSurfaceDisplay::present() {
    if (!initialized_ || !window_) {
        return false;
    }

    ANativeWindow_Buffer buffer{};
    ARect dirty{0, 0, static_cast<int32_t>(width_), static_cast<int32_t>(height_)};

    if (ANativeWindow_lock(window_, &buffer, &dirty) != 0) {
        return false;
    }

    ANativeWindow_unlockAndPost(window_);
    return true;
}

void AndroidSurfaceDisplay::shutdown() {
    initialized_ = false;
    width_ = 0;
    height_ = 0;
}

std::uint32_t AndroidSurfaceDisplay::width() const noexcept {
    return width_;
}

std::uint32_t AndroidSurfaceDisplay::height() const noexcept {
    return height_;
}

std::uint64_t AndroidSurfaceDisplay::memory_bytes() const noexcept {
    return 0;
}

} // namespace gaming_runtime
