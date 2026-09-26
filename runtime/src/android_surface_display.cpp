#include "gaming_runtime/android_surface_display.h"

#include <android/native_window.h>

#include <cstring>

namespace gaming_runtime {

AndroidSurfaceDisplay::~AndroidSurfaceDisplay() {
    clear_window();
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

bool AndroidSurfaceDisplay::present(const FramebufferView& framebuffer) {
    if (!initialized_ || !window_ || !framebuffer.valid() ||
        framebuffer.width != width_ || framebuffer.height != height_) {
        return false;
    }

    const std::uint64_t row_bytes =
        static_cast<std::uint64_t>(framebuffer.width) * 4ULL;
    if (framebuffer.stride_bytes < row_bytes) {
        return false;
    }

    ANativeWindow_Buffer buffer{};
    ARect dirty{0, 0, static_cast<int32_t>(width_), static_cast<int32_t>(height_)};

    if (ANativeWindow_lock(window_, &buffer, &dirty) != 0) {
        return false;
    }

    bool copied = buffer.bits != nullptr &&
                  buffer.width >= width_ &&
                  buffer.height >= height_ &&
                  buffer.stride >= static_cast<int32_t>(width_);

    if (copied) {
        const auto* source = framebuffer.pixels;
        auto* destination = static_cast<std::uint8_t*>(buffer.bits);
        const std::size_t destination_stride =
            static_cast<std::size_t>(buffer.stride) * 4U;

        for (std::uint32_t y = 0; y < height_; ++y) {
            std::memcpy(
                destination + static_cast<std::size_t>(y) * destination_stride,
                source + static_cast<std::size_t>(y) * framebuffer.stride_bytes,
                static_cast<std::size_t>(row_bytes));
        }
    }

    const int post_result = ANativeWindow_unlockAndPost(window_);
    return copied && post_result == 0;
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
