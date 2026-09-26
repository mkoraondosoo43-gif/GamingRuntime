#pragma once

#include "gaming_runtime/display.h"

struct ANativeWindow;

namespace gaming_runtime {

class AndroidSurfaceDisplay final : public DisplayBackend {
public:
    AndroidSurfaceDisplay() = default;
    ~AndroidSurfaceDisplay() override;

    AndroidSurfaceDisplay(const AndroidSurfaceDisplay&) = delete;
    AndroidSurfaceDisplay& operator=(const AndroidSurfaceDisplay&) = delete;

    bool set_window(ANativeWindow* window);
    void clear_window();

    bool initialize(std::uint32_t width, std::uint32_t height) override;
    bool resize(std::uint32_t width, std::uint32_t height) override;
    bool present(const FramebufferView& framebuffer) override;
    void shutdown() override;

    std::uint32_t width() const noexcept override;
    std::uint32_t height() const noexcept override;
    std::uint64_t memory_bytes() const noexcept override;

private:
    ANativeWindow* window_ = nullptr;
    bool initialized_ = false;
    std::uint32_t width_ = 0;
    std::uint32_t height_ = 0;
};

} // namespace gaming_runtime
