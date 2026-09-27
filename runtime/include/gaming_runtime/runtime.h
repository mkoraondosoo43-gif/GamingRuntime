#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

#include "gaming_runtime/asset_manager.h"
#include "gaming_runtime/audio.h"
#include "gaming_runtime/display.h"
#include "gaming_runtime/game_module.h"
#include "gaming_runtime/input.h"
#include "gaming_runtime/memory.h"
#include "gaming_runtime/frame_scheduler.h"
#include "gaming_runtime/render.h"
#include "gaming_runtime/entity.h"
#include "gaming_runtime/transform.h"
#include "gaming_runtime/renderable.h"
#include "gaming_runtime/camera.h"
#include "gaming_runtime/world.h"

namespace gaming_runtime {

struct RuntimeConfig {
    std::uint32_t target_fps = 60;
    std::uint32_t max_memory_mb = 1024;
    std::uint32_t supported_package_format = 1;
};

struct GamePackage {
    std::uint32_t format_version = 0;
    std::string id;
    std::string name;
    std::string version;
    std::string entry_point;
    std::string root_directory;
    std::string assets_directory;
    std::string bytecode_path;
    std::uint64_t estimated_memory_mb = 0;
};

struct FrameState {
    std::uint64_t frame_number = 0;
    double delta_seconds = 0.0;
    double frame_budget_seconds = 0.0;
    bool frame_over_budget = false;
};

class Runtime {
public:
    explicit Runtime(RuntimeConfig config = {});

    bool load_game(const GamePackage& package);
    bool load_manifest(const std::string& manifest_path);
    bool load_game_from_storage(const std::string& game_id,
                                const std::string& storage_root);

    bool attach_game_module(std::unique_ptr<GameModule> module);
    bool load_bytecode_module(const std::string& bytecode_path);
    bool start_game();
    void stop_game();

    bool can_run_locally() const;
    void tick(double delta_seconds);

    const FrameState& frame_state() const noexcept;
    const GamePackage& loaded_game() const noexcept;
    bool game_started() const noexcept;
    const AssetManager& assets() const noexcept;
    RenderFrame& render_frame() noexcept;
    const RenderFrame& render_frame() const noexcept;
    bool attach_renderer(std::unique_ptr<Renderer> renderer,
                         std::uint32_t width,
                         std::uint32_t height);
    bool load_texture_asset(std::uint32_t resource_id,
                            const std::string& relative_path,
                            std::uint32_t width,
                            std::uint32_t height);
    bool unload_texture(std::uint32_t resource_id);

    AudioFrame& audio_frame() noexcept;
    const AudioFrame& audio_frame() const noexcept;
    bool attach_audio(std::unique_ptr<AudioBackend> audio,
                      std::uint32_t sample_rate,
                      std::uint32_t channels);

    bool attach_display(std::unique_ptr<DisplayBackend> display,
                        std::uint32_t width,
                        std::uint32_t height);
    bool resize_display(std::uint32_t width, std::uint32_t height);
    void detach_display() noexcept;

    InputManager& input_manager() noexcept;
    const InputManager& input_manager() const noexcept;
    void set_input_button(std::size_t button, bool down);
    void set_input_axis(std::size_t axis, float value);
    void set_pointer_input(float x, float y, bool down);
    const InputState& input_state() const noexcept;
    const MemoryManager& memory() const noexcept;
    const FrameScheduler& frame_scheduler() const noexcept;
    GameWorld& world() noexcept;
    const GameWorld& world() const noexcept;

    EntityId create_entity();
    bool destroy_entity(EntityId entity);
    bool entity_alive(EntityId entity) const noexcept;
    Transform* transform(EntityId entity) noexcept;
    const Transform* transform(EntityId entity) const noexcept;
    std::size_t entity_count() const noexcept;
    Renderable* renderable(EntityId entity) noexcept;
    const Renderable* renderable(EntityId entity) const noexcept;
    Camera* camera(EntityId entity) noexcept;
    const Camera* camera(EntityId entity) const noexcept;
    bool attach_camera(EntityId entity);
    bool detach_camera(EntityId entity);
    std::size_t camera_count() const noexcept;

private:
    RuntimeConfig config_;
    GamePackage game_;
    FrameState frame_;
    std::unique_ptr<GameModule> game_module_;
    AssetManager asset_manager_;
    RenderFrame render_frame_;
    AudioFrame audio_frame_;
    InputManager input_manager_;
    MemoryManager memory_manager_;
    FrameScheduler frame_scheduler_;
    EntityManager entity_manager_;
    TransformManager transform_manager_;
    RenderableManager renderable_manager_;
    CameraManager camera_manager_;
    GameWorld world_;
    std::unordered_map<std::uint32_t, std::uint64_t> texture_memory_;
    std::unique_ptr<Renderer> renderer_;
    std::uint64_t renderer_memory_bytes_ = 0;
    std::unique_ptr<AudioBackend> audio_;
    std::uint64_t audio_memory_bytes_ = 0;
    std::unique_ptr<DisplayBackend> display_;
    std::uint64_t display_memory_bytes_ = 0;
    bool renderer_started_ = false;
    bool audio_started_ = false;
    bool display_started_ = false;
    bool game_loaded_ = false;
    bool game_started_ = false;
};

} // namespace gaming_runtime
