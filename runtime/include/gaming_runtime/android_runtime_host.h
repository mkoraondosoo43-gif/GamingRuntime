#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

struct AInputEvent;
struct ANativeWindow;

namespace gaming_runtime {

class AndroidInputBridge;
class Runtime;

class AndroidRuntimeHost final {
public:
    explicit AndroidRuntimeHost(Runtime& runtime);
    ~AndroidRuntimeHost();

    AndroidRuntimeHost(const AndroidRuntimeHost&) = delete;
    AndroidRuntimeHost& operator=(const AndroidRuntimeHost&) = delete;

    bool attach_surface(ANativeWindow* window);
    bool resize_surface();
    void detach_surface() noexcept;

    bool map_key(std::int32_t key_code, std::size_t button);
    bool map_axis(std::int32_t axis, std::size_t runtime_axis);
    void clear_input_mappings();

    bool handle_input_event(const AInputEvent* event);
    bool start();
    void stop() noexcept;
    void tick(double delta_seconds);

    bool surface_attached() const noexcept;
    std::uint32_t surface_width() const noexcept;
    std::uint32_t surface_height() const noexcept;

private:
    Runtime* runtime_ = nullptr;
    std::unique_ptr<AndroidInputBridge> input_bridge_;
    bool surface_attached_ = false;
    std::uint32_t surface_width_ = 0;
    std::uint32_t surface_height_ = 0;
};

} // namespace gaming_runtime
