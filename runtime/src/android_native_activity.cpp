#include "gaming_runtime/android_runtime_host.h"
#include "gaming_runtime/game_module.h"
#include "gaming_runtime/render.h"
#include "gaming_runtime/runtime.h"

#include <android/input.h>
#include <android/looper.h>
#include <android/native_activity.h>
#include <android/native_window.h>
#include <android/log.h>

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

constexpr char kLogTag[] = "GamingRuntime";

void log_message(const char* message) {
    __android_log_print(ANDROID_LOG_INFO, kLogTag, "%s", message);
}

bool paint_surface_probe(ANativeWindow* window, std::uint32_t width, std::uint32_t height) {
    if (!window || width == 0 || height == 0) {
        return false;
    }

    if (ANativeWindow_setBuffersGeometry(window, static_cast<int32_t>(width), static_cast<int32_t>(height), WINDOW_FORMAT_RGBA_8888) != 0) {
        log_message("surface probe: setBuffersGeometry failed");
        return false;
    }

    ANativeWindow_Buffer buffer{};
    ARect dirty{0, 0, static_cast<int32_t>(width), static_cast<int32_t>(height)};
    if (ANativeWindow_lock(window, &buffer, &dirty) != 0) {
        log_message("surface probe: lock failed");
        return false;
    }

    const bool valid = buffer.bits != nullptr && buffer.width >= width && buffer.height >= height && buffer.stride >= static_cast<int32_t>(width);
    if (valid) {
        auto* pixels = static_cast<std::uint8_t*>(buffer.bits);
        const std::size_t stride = static_cast<std::size_t>(buffer.stride) * 4U;
        for (std::uint32_t y = 0; y < height; ++y) {
            auto* row = pixels + static_cast<std::size_t>(y) * stride;
            for (std::uint32_t x = 0; x < width; ++x) {
                row[x * 4U + 0U] = 20U;
                row[x * 4U + 1U] = 220U;
                row[x * 4U + 2U] = 70U;
                row[x * 4U + 3U] = 255U;
            }
        }
    }

    const int post_result = ANativeWindow_unlockAndPost(window);
    log_message(valid && post_result == 0 ? "surface probe: POST OK" : "surface probe: POST FAILED");
    return valid && post_result == 0;
}

class AndroidDemoGame final : public GameModule {
public:
    bool initialize() override {
        return true;
    }

    void update(const GameFrameContext& context) override {
        if (!context.render) {
            return;
        }

        auto& frame = *context.render;
        // Deliberately bright startup frame so a working surface is obvious
        // on a real device; the software renderer uses pixel coordinates.
        frame.clear({0.08f, 0.18f, 0.35f, 1.0f});

        const float t = static_cast<float>(
            static_cast<double>(context.frame_number) * 0.03);
        const float x = 120.0f + 140.0f * std::sin(t);
        const float y = 160.0f + 100.0f * std::cos(t * 0.8f);

        frame.draw_quad(
            x,
            y,
            220.0f,
            220.0f,
            0);
    }

    void shutdown() override {}

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
    auto next_frame = previous;

    while (state->running.load(std::memory_order_acquire)) {
        const auto now = std::chrono::steady_clock::now();
        const std::chrono::duration<double> elapsed = now - previous;
        previous = now;

        {
            std::lock_guard<std::mutex> lock(state->mutex);
            state->host.tick(elapsed.count());
        }

        const double frame_budget =
            state->runtime.frame_scheduler().frame_budget_seconds();
        if (frame_budget > 0.0) {
            next_frame += std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                std::chrono::duration<double>(frame_budget));

            const auto after_tick = std::chrono::steady_clock::now();
            if (next_frame > after_tick) {
                std::this_thread::sleep_until(next_frame);
            } else {
                next_frame = after_tick;
            }
        }
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

    const int native_width = ANativeWindow_getWidth(window);
    const int native_height = ANativeWindow_getHeight(window);
    log_message("native window created");
    if (native_width <= 0 || native_height <= 0) {
        log_message("native window has invalid size");
        return;
    }

    paint_surface_probe(window, static_cast<std::uint32_t>(native_width), static_cast<std::uint32_t>(native_height));

    if (!state->host.attach_surface(window)) {
        log_message("runtime surface attach FAILED");
        return;
    }
    log_message("runtime surface attached");

    const std::uint32_t width = state->host.surface_width();
    const std::uint32_t height = state->host.surface_height();

    if (!state->runtime.attach_renderer(
            std::make_unique<SoftwareRenderer>(),
            width,
            height)) {
        log_message("software renderer attach FAILED");
        state->host.detach_surface();
        return;
    }
    log_message("software renderer attached");

    if (!state->host.start()) {
        log_message("runtime start FAILED");
        state->host.detach_surface();
        return;
    }
    log_message("runtime started");
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
    log_message("NativeActivity created");

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
        std::make_unique<AndroidDemoGame>());

    state->frame_thread = std::thread(frame_loop, state);
}
