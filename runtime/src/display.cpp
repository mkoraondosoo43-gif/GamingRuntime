#include "gaming_runtime/display.h"

#include <limits>

namespace gaming_runtime {

namespace {

bool valid_dimensions(std::uint32_t width, std::uint32_t height) {
    if (width == 0 || height == 0) {
        return false;
    }

    constexpr std::uint64_t max_pixels = 16ULL * 1024ULL * 1024ULL;
    return static_cast<std::uint64_t>(width) * height <= max_pixels;
}

} // namespace

bool NullDisplay::initialize(std::uint32_t width, std::uint32_t height) {
    if (!valid_dimensions(width, height)) {
        return false;
    }

    initialized_ = true;
    width_ = width;
    height_ = height;
    presented_frames_ = 0;
    return true;
}

bool NullDisplay::resize(std::uint32_t width, std::uint32_t height) {
    if (!initialized_ || !valid_dimensions(width, height)) {
        return false;
    }

    width_ = width;
    height_ = height;
    return true;
}

bool NullDisplay::present() {
    if (!initialized_ ||
        presented_frames_ == std::numeric_limits<std::uint64_t>::max()) {
        return false;
    }

    ++presented_frames_;
    return true;
}

void NullDisplay::shutdown() {
    initialized_ = false;
    width_ = 0;
    height_ = 0;
    presented_frames_ = 0;
}

std::uint32_t NullDisplay::width() const noexcept {
    return width_;
}

std::uint32_t NullDisplay::height() const noexcept {
    return height_;
}

std::uint64_t NullDisplay::memory_bytes() const noexcept {
    return 0;
}

std::uint64_t NullDisplay::presented_frames() const noexcept {
    return presented_frames_;
}

} // namespace gaming_runtime
