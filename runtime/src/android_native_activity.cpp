#include "gaming_runtime/android_runtime_host.h"
#include "gaming_runtime/game_module.h"
#include "gaming_runtime/render.h"
#include "gaming_runtime/runtime.h"

#include <android/input.h>
#include <android/looper.h>
#include <android/native_activity.h>
#include <android/native_window.h>

#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

namespace gaming_runtime {
namespace {

class AndroidDemoGame final : public GameModule {
public:
    explicit AndroidDemoGame(Runtime& runtime) : runtime_(&runtime) {}

    bool initialize() override {
        return runtime_ != nullptr;
    }

    void update(const GameFrameContext& context) override {
        if (!runtime_) {
            return;
        }

        auto& frame = runtime_->render_frame();
        frame.clear({0.015f, 0.02f, 0.035f, 1.0f});

        const float t = static_cast<float>(
            static_cast<double>(context.frame_number) * 0.02);
        const float x = 0.5f + 0.25f * std::sin(t);
        const float y = 0.5f + 0.25f * std::cos(t * 0.8f);

        frame.draw_quad(
            x - 0.08f,
            y - 0.08f,
            0.16f,
            0.16f,
            0);
    }

    void shutdown() override {}

private:
    Runtime* runtime_ = nullptr;
};

struct HostState {
    Runtime runtime;
    AndroidRuntimeHost host;
    std::mutex mutex;
    std::atomic<bool> running{true};
    std::thread frame_thread;
    std::thread input_thread;
    AInputQueue* input_queue = nullptr;
    ALooper* input_looper = nullptr;

    HostState() : runtime(RuntimeConfig{}), host(runtime) {}
};

bool write_demo_package(const std::string& root) {
    const std::filesystem::path package =
        std::filesystem::path(root) / "demo_game";
    const std::filesystem::path assets = package / "assets";

    std::error_code error;
    std::filesystem::create_directories(assets, error);
    if (error) {
        return false;
    }

    std::ofstream manifest(package / "game.json", std::ios::binary);
    std::ofstream bytecode(package / "game.bc", std::ios::binary);
    if (!manifest || !bytecode) {
        return false;
    }

    manifest
        << "{\n"
        << "  \"format_version\": 1,\n"
        << "  \"id\": \"android.demo\",\n"
        << "  \"name\": \"GamingRuntime Android Demo\",\n"
        << "  \"version\": \"2.5.0\",\n"
        << "  \"entry_point\": \"main\",\n"
        << "  \"bytecode\": \"game.bc\",\n"
        << "  \"assets\": \"assets\",\n"
        << "  \"estimated_memory_mb\": 32\n"
        << "}\n";

    bytecode << "HALT\n";
    return static_cast<bool>(manifest) && static_cast<bool>(bytecode);
}

void frame_loop(HostState* state) {
    auto previous = std::chrono::steady_clock::now();

    while (state->running.load(std::memory_order_acquire)) {
        const auto now = std::chrono::steady_clock::now();
        const std::chrono::duration<double> elapsed = now - previous;
        previous = now;

        {
            std::lock_guard<std::mutex> lock(state->mutex);
            state->host.tick(elapsed.count());
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void input_loop(HostState* state, AInputQueue* queue) {
    ALooper* looper =
        ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS);
    state->input_looper = looper;

    AInputQueue_attachLooper(queue, looper, 1, nullptr, nullptr);

    while (state->running.load(std::memory_order_acquire) &&
           state->input_queue == queue) {
        const int poll_result =
            ALooper_pollOnce(-1, nullptr, nullptr, nullptr);
        if (poll_result != 1) {
            continue;
        }

        AInputEvent* event = nullptr;
        while (AInputQueue_getEvent(queue, &event) >= 0) {
            if (AInputQueue_preDispatchEvent(queue, event)) {
                continue;
            }

            {
                std::lock_guard<std::mutex> lock(state->mutex);
                state->host.handle_input_event(event);
            }

            AInputQueue_finishEvent(queue, event, 1);
        }
    }

    AInputQueue_detachLooper(queue);
}

void stop_input_thread(HostState* state) {
    if (state->input_looper) {
        ALooper_wake(state->input_looper);
    }

    if (state->input_thread.joinable()) {
        state->input_thread.join();
    }

    state->input_looper = nullptr;
}

void native_window_created(ANativeActivity* activity, ANativeWindow* window) {
    auto* state = static_cast<HostState*>(activity->instance);
    if (!state || !window) {
        return;
    }

    std::lock_guard<std::mutex> lock(state->mutex);

    if (!state->host.attach_surface(window)) {
        return;
    }

    const std::uint32_t width = state->host.surface_width();
    const std::uint32_t height = state->host.surface_height();

    if (!state->runtime.attach_renderer(
            std::make_unique<SoftwareRenderer>(),
            width,
            height)) {
        state->host.detach_surface();
        return;
    }

    state->host.start();
}

void native_window_resized(ANativeActivity* activity, ANativeWindow* window) {
    auto* state = static_cast<HostState*>(activity->instance);
    if (!state || !window) {
        return;
    }

    const int width = ANativeWindow_getWidth(window);
    const int height = ANativeWindow_getHeight(window);

    if (width <= 0 || height <= 0) {
        return;
    }

    std::lock_guard<std::mutex> lock(state->mutex);
    state->host.resize_surface(
        static_cast<std::uint32_t>(width),
        static_cast<std::uint32_t>(height));
}

void native_window_destroyed(ANativeActivity* activity, ANativeWindow*) {
    auto* state = static_cast<HostState*>(activity->instance);
    if (!state) {
        return;
    }

    std::lock_guard<std::mutex> lock(state->mutex);
    state->host.stop();
    state->host.detach_surface();
}

void input_queue_created(ANativeActivity* activity, AInputQueue* queue) {
    auto* state = static_cast<HostState*>(activity->instance);
    if (!state || !queue) {
        return;
    }

    stop_input_thread(state);

    state->input_queue = queue;
    state->input_thread = std::thread(input_loop, state, queue);
}

void input_queue_destroyed(ANativeActivity* activity, AInputQueue* queue) {
    auto* state = static_cast<HostState*>(activity->instance);
    if (!state || state->input_queue != queue) {
        return;
    }

    state->input_queue = nullptr;
    stop_input_thread(state);
}

void on_destroy(ANativeActivity* activity) {
    auto* state = static_cast<HostState*>(activity->instance);
    if (!state) {
        return;
    }

    state->running.store(false, std::memory_order_release);
    if (state->input_looper) {
        ALooper_wake(state->input_looper);
    }

    if (state->frame_thread.joinable()) {
        state->frame_thread.join();
    }

    if (state->input_thread.joinable()) {
        state->input_thread.join();
    }

    {
        std::lock_guard<std::mutex> lock(state->mutex);
        state->host.stop();
        state->host.detach_surface();
    }

    delete state;
    activity->instance = nullptr;
}

} // namespace

} // namespace gaming_runtime

extern "C" void ANativeActivity_onCreate(
    ANativeActivity* activity,
    void* saved_state,
    size_t saved_state_size) {
    (void)saved_state;
    (void)saved_state_size;

    using namespace gaming_runtime;

    auto* state = new HostState();
    activity->instance = state;

    activity->callbacks->onNativeWindowCreated = native_window_created;
    activity->callbacks->onNativeWindowResized = native_window_resized;
    activity->callbacks->onNativeWindowDestroyed = native_window_destroyed;
    activity->callbacks->onInputQueueCreated = input_queue_created;
    activity->callbacks->onInputQueueDestroyed = input_queue_destroyed;
    activity->callbacks->onDestroy = on_destroy;

    const std::string games_root =
        std::string(activity->internalDataPath) + "/games";

    if (!write_demo_package(games_root) ||
        !state->runtime.load_game_from_storage("android.demo", games_root)) {
        state->running.store(false, std::memory_order_release);
        return;
    }

    state->runtime.attach_game_module(
        std::make_unique<AndroidDemoGame>(state->runtime));

    state->frame_thread = std::thread(frame_loop, state);
}
