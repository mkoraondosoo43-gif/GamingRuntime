#include "gaming_runtime/runtime.h"
#include "gaming_runtime/bytecode.h"
#include "gaming_runtime/asset_manager.h"
#include "gaming_runtime/render.h"
#include "gaming_runtime/audio.h"
#include "gaming_runtime/display.h"
#include "gaming_runtime/memory.h"

#include <cassert>
#include <memory>
#include <filesystem>
#include <fstream>
#include <vector>
#include <cmath>

class WorldAwareGame final : public gaming_runtime::GameModule {
public:
    bool initialize() override { return true; }

    void update(const gaming_runtime::GameFrameContext& context) override {
        assert(context.world != nullptr);
        entity = context.world->create_entity();
        assert(entity != gaming_runtime::kInvalidEntity);

        auto* transform = context.world->transform(entity);
        assert(transform != nullptr);
        transform->position = {4.0f, 5.0f, 6.0f};

        auto* renderable = context.world->renderable(entity);
        assert(renderable != nullptr);
        assert(renderable->visible);
        renderable->resource_id = 7;

        observed = true;
    }

    void shutdown() override {}

    gaming_runtime::EntityId entity = gaming_runtime::kInvalidEntity;
    bool observed = false;
};

int main() {
    {
        gaming_runtime::MemoryManager memory(1);
        assert(memory.budget_bytes() == 1024ULL * 1024ULL);
        assert(memory.used_bytes() == 0);
        assert(memory.available_bytes() == memory.budget_bytes());
        assert(memory.reserve(512ULL * 1024ULL));
        assert(memory.used_bytes() == 512ULL * 1024ULL);
        assert(memory.available_bytes() == 512ULL * 1024ULL);
        assert(!memory.reserve(512ULL * 1024ULL + 1ULL));
        assert(memory.used_bytes() == 512ULL * 1024ULL);
        memory.release(128ULL * 1024ULL);
        assert(memory.used_bytes() == 384ULL * 1024ULL);
        memory.release(UINT64_MAX);
        assert(memory.used_bytes() == 0);
        memory.clear();
        assert(memory.available_bytes() == memory.budget_bytes());
    }

    const std::filesystem::path root = "runtime_test_library";
    const std::filesystem::path game = root / "demo_game";
    const std::filesystem::path assets = game / "assets";

    std::filesystem::remove_all(root);
    std::filesystem::create_directories(assets / "textures");
    std::filesystem::create_directories(assets / "audio");

    {
        std::ofstream texture(assets / "textures" / "hero.bin", std::ios::binary);
        texture << "ASSET";
    }

    {
        std::ofstream audio(assets / "audio" / "hit.bin", std::ios::binary);
        audio << "HIT";
    }

    {
        const std::uint8_t texture[] = {
            255, 0, 0, 255,
            0, 255, 0, 255,
            0, 0, 255, 255,
            255, 255, 0, 255
        };
        std::ofstream file(assets / "textures" / "checker.rgba", std::ios::binary);
        file.write(reinterpret_cast<const char*>(texture), sizeof(texture));
    }

    {
        const std::uint8_t texture[] = {255, 0, 0, 255};
        std::ofstream file(assets / "textures" / "tiny.rgba", std::ios::binary);
        file.write(reinterpret_cast<const char*>(texture), sizeof(texture));
    }

    {
        std::ofstream manifest(game / "game.json");
        manifest << R"({
            "format_version": 1,
            "id": "demo.game",
            "name": "Runtime Demo",
            "version": "0.8.0",
            "entry_point": "main",
            "bytecode": "game.bc",
            "assets": "assets",
            "estimated_memory_mb": 128
        })";
    }

    const std::filesystem::path bytecode = game / "game.bc";
    {
        std::ofstream program(bytecode);
        program << "PUSH 20\n";
        program << "PUSH 22\n";
        program << "ADD\n";
        program << "SET 0\n";
        program << "HALT\n";
    }

    gaming_runtime::Runtime runtime({60, 512, 1});

    assert(runtime.load_game_from_storage(
        "demo.game",
        root.string()));

    assert(runtime.loaded_game().format_version == 1);
    assert(runtime.loaded_game().id == "demo.game");
    assert(runtime.loaded_game().version == "0.8.0");
    assert(runtime.loaded_game().root_directory ==
           std::filesystem::weakly_canonical(game).string());
    assert(runtime.loaded_game().assets_directory ==
           std::filesystem::weakly_canonical(assets).string());
    assert(runtime.can_run_locally());
    assert(runtime.memory().used_bytes() == 128ULL * 1024ULL * 1024ULL);
    assert(runtime.memory().available_bytes() ==
           (512ULL - 128ULL) * 1024ULL * 1024ULL);
    runtime.set_input_button(2, true);
    runtime.set_input_axis(0, 1.5f);
    runtime.set_pointer_input(120.0f, 80.0f, true);
    assert(runtime.input_state().button_down(2));
    assert(runtime.input_state().axis_value(0) == 1.0f);
    assert(runtime.input_state().pointer_x == 120.0f);
    assert(runtime.input_state().pointer_y == 80.0f);
    assert(runtime.input_state().pointer_down);
    runtime.set_input_button(63, true);
    runtime.set_input_axis(7, -2.0f);
    runtime.set_input_button(64, true);
    runtime.set_input_axis(8, 2.0f);
    assert(runtime.input_state().button_down(63));
    assert(runtime.input_state().axis_value(7) == -1.0f);
    assert(!runtime.input_state().button_down(64));
    assert(runtime.input_state().axis_value(8) == 0.0f);
    assert(runtime.assets().has_asset("textures/hero.bin"));
    assert(!runtime.assets().has_asset("../game.json"));

    const std::filesystem::path outside_bytecode = root / "outside.bc";
    {
        std::ofstream outside(outside_bytecode);
        outside << "PUSH 999\\nHALT\\n";
    }
    std::error_code bytecode_symlink_error;
    std::filesystem::create_symlink(
        outside_bytecode,
        game / "escape.bc",
        bytecode_symlink_error);
    if (!bytecode_symlink_error) {
        std::ofstream escaped_manifest(game / "game.json");
        escaped_manifest << R"({
            "format_version": 1,
            "id": "demo.game",
            "name": "Runtime Demo",
            "version": "0.5.0",
            "entry_point": "main",
            "bytecode": "escape.bc",
            "assets": "assets",
            "estimated_memory_mb": 128
        })";
        assert(!runtime.load_manifest((game / "game.json").string()));
    }

    const std::filesystem::path outside_asset = root / "outside.bin";
    {
        std::ofstream outside(outside_asset, std::ios::binary);
        outside << "SECRET";
    }
    std::error_code symlink_error;
    std::filesystem::create_symlink(
        outside_asset,
        assets / "textures" / "escape.bin",
        symlink_error);
    if (!symlink_error) {
        assert(!runtime.assets().has_asset("textures/escape.bin"));
        std::vector<std::uint8_t> escaped_data;
        assert(!runtime.assets().read_asset("textures/escape.bin", escaped_data));
    }
    std::vector<std::uint8_t> asset_data;
    assert(runtime.assets().read_asset("audio/hit.bin", asset_data));
    assert(asset_data.size() == 3);
    assert(runtime.assets().list_assets().size() == 4);

    {
        std::ofstream invalid_manifest(game / "game.json");
        invalid_manifest << R"({
            "format_version": 1,
            "id": "demo.game",
            "name": "Runtime Demo",
            "version": "0.5.0",
            "entry_point": "main",
            "bytecode": "game.bc",
            "assets": "../outside",
            "estimated_memory_mb": 128
        })";
    }

    assert(!runtime.load_manifest((game / "game.json").string()));

    {
        std::ofstream valid_manifest(game / "game.json");
        valid_manifest << R"({
            "format_version": 1,
            "id": "demo.game",
            "name": "Runtime Demo",
            "version": "0.5.0",
            "entry_point": "main",
            "bytecode": "game.bc",
            "assets": "assets",
            "estimated_memory_mb": 128
        })";
    }

    assert(runtime.load_manifest((game / "game.json").string()));
    runtime.set_input_button(2, true);

    // Strict manifest parsing: escaped strings are decoded and trailing garbage is rejected.
    {
        std::ofstream escaped(game / "game.json");
        escaped << R"({
            "format_version": 1,
            "id": "demo\\.game",
            "name": "Runtime \"Demo\"",
            "version": "0.5.0",
            "entry_point": "main",
            "bytecode": "game.bc",
            "assets": "assets",
            "estimated_memory_mb": 128
        })";
    }
    assert(runtime.load_manifest((game / "game.json").string()));
    assert(runtime.loaded_game().id == "demo\\.game");
    assert(runtime.loaded_game().name == "Runtime \"Demo\"");

    {
        std::ofstream malformed(game / "game.json");
        malformed << "{\\\"format_version\\\":1,\\\"id\\\":\\\"bad\\\"} trailing";
    }
    assert(!runtime.load_manifest((game / "game.json").string()));
    assert(runtime.loaded_game().id == "demo\\.game");

    {
        std::ofstream bad_bytecode(game / "bad.bc");
        bad_bytecode << "NOT_AN_OPCODE\\n";
        std::ofstream bad_manifest(game / "game.json");
        bad_manifest << R"({
            "format_version": 1,
            "id": "bad.game",
            "name": "Bad",
            "version": "1",
            "entry_point": "main",
            "bytecode": "bad.bc",
            "assets": "assets",
            "estimated_memory_mb": 128
        })";
    }
    assert(!runtime.load_manifest((game / "game.json").string()));
    assert(runtime.loaded_game().id == "demo\\.game");

    {
        std::ofstream valid_manifest(game / "game.json");
        valid_manifest << R"({
            "format_version": 1,
            "id": "demo.game",
            "name": "Runtime Demo",
            "version": "0.5.0",
            "entry_point": "main",
            "bytecode": "game.bc",
            "assets": "assets",
            "estimated_memory_mb": 128
        })";
    }
    assert(runtime.load_manifest((game / "game.json").string()));
    runtime.set_input_button(2, true);
    assert(runtime.memory().used_bytes() == 128ULL * 1024ULL * 1024ULL);

    auto renderer = std::make_unique<gaming_runtime::SoftwareRenderer>();
    auto* renderer_ptr = renderer.get();
    assert(runtime.attach_renderer(std::move(renderer), 1280, 720));

    auto audio = std::make_unique<gaming_runtime::NullAudio>();
    auto* audio_ptr = audio.get();
    assert(runtime.attach_audio(std::move(audio), 48000, 2));
    assert(audio_ptr->sample_rate() == 48000);
    assert(audio_ptr->channels() == 2);

    auto display = std::make_unique<gaming_runtime::NullDisplay>();
    auto* display_ptr = display.get();
    assert(runtime.attach_display(std::move(display), 1280, 720));
    assert(display_ptr->width() == 1280);
    assert(display_ptr->height() == 720);
    assert(runtime.render_frame().size() == 0);
    assert(runtime.render_frame().clear({0.02f, 0.03f, 0.05f, 1.0f}));
    assert(runtime.render_frame().draw_quad(10.0f, 20.0f, 100.0f, 50.0f, 7));
    assert(runtime.render_frame().size() == 2);

    class TestGame final : public gaming_runtime::GameModule {
    public:
        TestGame(gaming_runtime::RenderFrame& frame, gaming_runtime::AudioFrame& audio) : frame(frame), audio(audio) {}

        bool initialize() override {
            initialized = true;
            return true;
        }

        void update(const gaming_runtime::GameFrameContext& context) override {
            frame.clear({0.0f, 0.0f, 0.0f, 1.0f});
            frame.draw_quad(2.0f, 3.0f, 10.0f, 8.0f, 7);
            audio.play(1, 0.75f, false);
            last_frame = context.frame_number;
            last_delta = context.delta_seconds;
            assert(context.input != nullptr);
            assert(context.input->button_down(2));
        }

        void shutdown() override {
            shutdown_called = true;
        }

        bool initialized = false;
        bool shutdown_called = false;
        std::uint64_t last_frame = 0;
        double last_delta = 0.0;
        gaming_runtime::RenderFrame& frame;
        gaming_runtime::AudioFrame& audio;
    };

    auto module = std::make_unique<TestGame>(runtime.render_frame(), runtime.audio_frame());
    auto* module_ptr = module.get();

    assert(runtime.attach_game_module(std::move(module)));
    assert(runtime.start_game());
    assert(runtime.game_started());
    assert(module_ptr->initialized);

    runtime.tick(1.0 / 60.0);
    assert(runtime.frame_state().frame_number == 1);
    assert(runtime.frame_state().delta_seconds > 0.0);
    assert(runtime.frame_state().frame_budget_seconds > 0.0);
    assert(!runtime.frame_state().frame_over_budget);
    assert(runtime.frame_state().frame_budget_seconds == runtime.frame_scheduler().frame_budget_seconds());

    runtime.tick(1.0 / 30.0);
    assert(runtime.frame_state().frame_number == 2);
    assert(runtime.frame_state().frame_over_budget);
    assert(renderer_ptr->submitted_frames() == 2);
    assert(renderer_ptr->framebuffer().valid());
    assert(renderer_ptr->framebuffer().width == 1280);
    assert(renderer_ptr->framebuffer().height == 720);
    assert(renderer_ptr->framebuffer().size_bytes == 1280ULL * 720ULL * 4ULL);
    assert(audio_ptr->submitted_frames() == 2);
    assert(audio_ptr->last_command_count() == 1);
    assert(display_ptr->presented_frames() == 2);
    assert(display_ptr->last_framebuffer_bytes() == 1280ULL * 720ULL * 4ULL);
    assert(module_ptr->last_frame == 2);
    assert(module_ptr->last_delta > 0.0);

    const auto memory_before_resize = runtime.memory().used_bytes();
    assert(runtime.resize_display(640, 360));
    assert(display_ptr->width() == 640);
    assert(display_ptr->height() == 360);
    assert(renderer_ptr->framebuffer().width == 640);
    assert(renderer_ptr->framebuffer().height == 360);
    assert(renderer_ptr->framebuffer().size_bytes ==
           640ULL * 360ULL * 4ULL);
    assert(runtime.memory().used_bytes() < memory_before_resize);

    runtime.stop_game();
    assert(!runtime.game_started());
    assert(module_ptr->shutdown_called);

    {
        const auto camera_entity = runtime.create_entity();
        assert(camera_entity != gaming_runtime::kInvalidEntity);
        assert(runtime.attach_camera(camera_entity));
        const auto* camera = runtime.camera(camera_entity);
        assert(camera != nullptr);
        assert(camera->viewport_width == 640);
        assert(camera->viewport_height == 360);

        assert(runtime.resize_display(320, 180));
        camera = runtime.camera(camera_entity);
        assert(camera != nullptr);
        assert(camera->viewport_width == 320);
        assert(camera->viewport_height == 180);

        assert(runtime.resize_display(640, 360));
        camera = runtime.camera(camera_entity);
        assert(camera != nullptr);
        assert(camera->viewport_width == 640);
        assert(camera->viewport_height == 360);
        assert(runtime.destroy_entity(camera_entity));
    }

    {
        auto software = std::make_unique<gaming_runtime::SoftwareRenderer>();
        auto* software_ptr = software.get();
        assert(runtime.attach_renderer(std::move(software), 8, 8));
        const auto memory_before_texture = runtime.memory().used_bytes();
        assert(runtime.load_texture_asset(12, "textures/checker.rgba", 2, 2));
        assert(software_ptr->has_texture(12));
        assert(runtime.memory().used_bytes() ==
               memory_before_texture + 16ULL);
        assert(runtime.render_frame().clear({0.0f, 0.0f, 0.0f, 1.0f}));
        assert(runtime.render_frame().draw_quad(0.0f, 0.0f, 4.0f, 4.0f, 12));
        assert(software_ptr->submit(runtime.render_frame()));

        const auto& checker_pixels = software_ptr->pixels();
        assert(checker_pixels[0] == 255);
        assert(checker_pixels[1] == 0);
        assert(checker_pixels[2] == 0);
        const std::size_t green_pixel = (0U * 8U + 2U) * 4U;
        assert(checker_pixels[green_pixel] == 0);
        assert(checker_pixels[green_pixel + 1] == 255);
        assert(checker_pixels[green_pixel + 2] == 0);

        assert(runtime.load_texture_asset(12, "textures/tiny.rgba", 1, 1));
        assert(runtime.memory().used_bytes() ==
               memory_before_texture + 4ULL);
        assert(runtime.render_frame().clear({0.0f, 0.0f, 0.0f, 1.0f}));
        assert(runtime.render_frame().draw_quad(0.0f, 0.0f, 4.0f, 4.0f, 12));
        assert(software_ptr->submit(runtime.render_frame()));

        const auto& tiny_pixels = software_ptr->pixels();
        assert(tiny_pixels[0] == 255);
        assert(tiny_pixels[1] == 0);
        assert(tiny_pixels[2] == 0);
        assert(runtime.unload_texture(12));
        assert(!software_ptr->has_texture(12));
        assert(runtime.memory().used_bytes() == memory_before_texture);
        assert(!runtime.unload_texture(12));
    }

    {
        gaming_runtime::Runtime constrained({60, 128, 1});
        assert(constrained.load_game_from_storage("demo.game", root.string()));
        auto constrained_renderer = std::make_unique<gaming_runtime::NullRenderer>();
        assert(constrained.attach_renderer(std::move(constrained_renderer), 8, 8));
        const auto before = constrained.memory().used_bytes();
        assert(!constrained.load_texture_asset(1, "textures/checker.rgba", 2, 2));
        assert(constrained.memory().used_bytes() == before);
    }

    {
        gaming_runtime::Runtime constrained({60, 128, 1});
        assert(constrained.load_game_from_storage("demo.game", root.string()));
        auto renderer = std::make_unique<gaming_runtime::SoftwareRenderer>();
        assert(renderer->initialize(128, 128));
        const auto before = constrained.memory().used_bytes();
        assert(!constrained.attach_renderer(std::move(renderer), 128, 128));
        assert(constrained.memory().used_bytes() == before);
    }

    {
        gaming_runtime::SoftwareRenderer software;
        assert(software.initialize(32, 24));
        gaming_runtime::Texture texture{
            2,
            2,
            {
                255, 0, 0, 255,
                0, 255, 0, 255,
                0, 0, 255, 255,
                255, 255, 0, 255
            }
        };
        assert(software.upload_texture(9, texture));
        assert(software.has_texture(9));
        gaming_runtime::RenderFrame frame;
        assert(frame.clear({0.1f, 0.2f, 0.3f, 1.0f}));
        assert(frame.draw_quad(4.0f, 5.0f, 8.0f, 6.0f, 9));
        assert(software.submit(frame));
        assert(software.width() == 32);
        assert(software.height() == 24);
        assert(software.pixels().size() == 32U * 24U * 4U);
        const auto framebuffer = software.framebuffer();
        assert(framebuffer.valid());
        assert(framebuffer.width == 32);
        assert(framebuffer.height == 24);
        assert(framebuffer.stride_bytes == 32U * 4U);
        assert(framebuffer.size_bytes == 32U * 24U * 4U);
        assert(framebuffer.pixels == software.pixels().data());
        const auto& pixels = software.pixels();
        const std::size_t quad_pixel = (5U * 32U + 4U) * 4U;
        assert(pixels[quad_pixel] == 255);
        assert(pixels[quad_pixel + 1] == 0);
        assert(pixels[quad_pixel + 2] == 0);
        assert(pixels[quad_pixel + 3] == 255);
        assert(software.unregister_texture(9));
        assert(!software.has_texture(9));
        software.shutdown();
        assert(software.pixels().empty());
    }


    {
        gaming_runtime::AudioFrame frame(2);
        assert(frame.play(1, 2.0f, true));
        assert(frame.set_master_volume(-1.0f));
        assert(!frame.stop_all());
        assert(frame.size() == 2);
        assert(frame.commands()[0].volume == 1.0f);
        assert(frame.commands()[0].loop);
        assert(frame.commands()[1].volume == 0.0f);
        frame.reset();
        assert(frame.size() == 0);
    }

    {
        const std::filesystem::path branch_program = game / "branch.bc";
        {
            std::ofstream program(branch_program);
            program << "PUSH 7\n";
            program << "SET 0\n";
            program << "GET 0\n";
            program << "PUSH 7\n";
            program << "EQ\n";
            program << "JZ 10\n";
            program << "PUSH 42\n";
            program << "SET 1\n";
            program << "HALT\n";
            program << "PUSH 99\n";
            program << "SET 1\n";
            program << "HALT\n";
        }

        gaming_runtime::BytecodeGameModule module(branch_program.string());
        assert(module.initialize());
        module.update({1, 1.0 / 60.0});
        assert(module.register_value(0) == 7);
        assert(module.register_value(1) == 42);
        assert(module.halted());
        module.shutdown();
    }

    {
        const std::filesystem::path entity_program = game / "entity.bc";
        {
            std::ofstream program(entity_program);
            program << "CREATE_ENTITY 0\n";
            program << "PUSH 10000\n";
            program << "PUSH 20000\n";
            program << "PUSH 30000\n";
            program << "SET_POSITION 0\n";
            program << "PUSH 2000\n";
            program << "PUSH 3000\n";
            program << "PUSH 4000\n";
            program << "SET_SCALE 0\n";
            program << "PUSH 7\n";
            program << "SET_RESOURCE 0\n";
            program << "PUSH 0\n";
            program << "SET_VISIBLE 0\n";
            program << "DESTROY_ENTITY 0\n";
            program << "HALT\n";
        }

        gaming_runtime::EntityManager entities;
        gaming_runtime::TransformManager transforms;
        gaming_runtime::RenderableManager renderables;
        gaming_runtime::CameraManager cameras;
        gaming_runtime::GameWorld world(entities, transforms, renderables, cameras);
        gaming_runtime::BytecodeGameModule module(entity_program.string());
        assert(module.initialize());

        gaming_runtime::GameFrameContext context;
        context.world = &world;
        module.update(context);

        assert(world.entity_count() == 0);
        assert(module.halted());
        module.shutdown();
    }

    {
        const std::filesystem::path memory_program = game / "memory.bc";
        {
            std::ofstream program(memory_program);
            program << "PUSH 1234\n";
            program << "STORE 5\n";
            program << "LOAD 5\n";
            program << "SET 2\n";
            program << "HALT\n";
        }

        gaming_runtime::BytecodeGameModule module(memory_program.string());
        assert(module.initialize());
        module.update({1, 1.0 / 60.0});
        assert(module.memory_value(5) == 1234);
        assert(module.register_value(2) == 1234);
        module.shutdown();
    }

    {
        const std::filesystem::path camera_program = game / "camera.bc";
        {
            std::ofstream program(camera_program);
            program << "CREATE_ENTITY 0\n";
            program << "ATTACH_CAMERA 0\n";
            program << "PUSH 2000\n";
            program << "SET_CAMERA_ZOOM 0\n";
            program << "PUSH 1280\n";
            program << "PUSH 720\n";
            program << "SET_CAMERA_VIEWPORT 0\n";
            program << "PUSH 0\n";
            program << "SET_CAMERA_ACTIVE 0\n";
            program << "PUSH 1\n";
            program << "SET_CAMERA_ACTIVE 0\n";
            program << "DETACH_CAMERA 0\n";
            program << "DESTROY_ENTITY 0\n";
            program << "HALT\n";
        }

        gaming_runtime::EntityManager entities;
        gaming_runtime::TransformManager transforms;
        gaming_runtime::RenderableManager renderables;
        gaming_runtime::CameraManager cameras;
        gaming_runtime::GameWorld world(entities, transforms, renderables, cameras);
        gaming_runtime::BytecodeGameModule module(camera_program.string());
        assert(module.initialize());

        gaming_runtime::GameFrameContext context;
        context.world = &world;
        module.update(context);

        assert(world.entity_count() == 0);
        assert(world.camera_count() == 0);
        assert(module.halted());
        module.shutdown();
    }

    {
        const auto entity = runtime.create_entity();
        assert(entity != gaming_runtime::kInvalidEntity);
        assert(runtime.entity_alive(entity));
        assert(runtime.entity_count() == 1);

        auto* transform = runtime.transform(entity);
        assert(transform != nullptr);
        transform->position = {10.0f, 20.0f, 30.0f};
        transform->scale = {2.0f, 2.0f, 2.0f};

        const auto* read_only = runtime.transform(entity);
        assert(read_only != nullptr);
        assert(read_only->position.y == 20.0f);
        assert(read_only->scale.z == 2.0f);

        assert(runtime.attach_camera(entity));
        assert(runtime.camera_count() == 1);
        auto* camera = runtime.camera(entity);
        assert(camera != nullptr);
        assert(camera->zoom == 1.0f);
        camera->zoom = 2.0f;
        camera->viewport_width = 1280;
        camera->viewport_height = 720;
        assert(runtime.camera(entity)->zoom == 2.0f);
        assert(runtime.camera(entity)->viewport_width == 1280);
        assert(runtime.camera(entity)->viewport_height == 720);
        assert(runtime.detach_camera(entity));
        assert(runtime.camera_count() == 0);
        assert(runtime.camera(entity) == nullptr);

        assert(runtime.destroy_entity(entity));
        assert(!runtime.entity_alive(entity));
        assert(runtime.transform(entity) == nullptr);
        assert(runtime.entity_count() == 0);
        assert(!runtime.destroy_entity(entity));
    }

    {
        gaming_runtime::EntityManager entities;
        gaming_runtime::TransformManager transforms;
        gaming_runtime::RenderableManager renderables;
        gaming_runtime::CameraManager cameras;
        gaming_runtime::GameWorld world(entities, transforms, renderables, cameras);

        const auto camera_entity = world.create_entity();
        const auto target_entity = world.create_entity();
        assert(camera_entity != gaming_runtime::kInvalidEntity);
        assert(target_entity != gaming_runtime::kInvalidEntity);
        assert(world.attach_camera(camera_entity));
        assert(world.camera_count() == 1);

        auto* camera_transform = world.transform(camera_entity);
        auto* camera = world.camera(camera_entity);
        auto* target_transform = world.transform(target_entity);
        assert(camera_transform != nullptr);
        assert(camera != nullptr);
        assert(target_transform != nullptr);

        camera_transform->position = {10.0f, 20.0f, 0.0f};
        camera->zoom = 2.0f;
        camera->viewport_width = 100;
        camera->viewport_height = 80;

        target_transform->position = {15.0f, 25.0f, 0.0f};
        target_transform->scale = {4.0f, 6.0f, 1.0f};
        target_transform->rotation.z = 0.25f;
        camera_transform->rotation.z = 0.1f;
        world.renderable(target_entity)->resource_id = 9;

        gaming_runtime::RenderFrame frame;
        assert(world.render(frame) == 2);
        assert(frame.size() == 2);

        const auto& commands = frame.commands();
        const auto& target_command = commands[1];
        assert(target_command.type == gaming_runtime::RenderCommand::Type::DrawQuad);
        assert(std::fabs(target_command.x - 60.9467f) < 0.001f);
        assert(std::fabs(target_command.y - 48.9549f) < 0.001f);
        assert(target_command.width == 8.0f);
        assert(target_command.height == 12.0f);
        assert(target_command.rotation == 0.15f);
        assert(target_command.resource_id == 9);
    }

    assert(runtime.load_bytecode_module(bytecode.string()));
    auto world_module = std::make_unique<WorldAwareGame>();
    auto* world_module_ptr = world_module.get();
    assert(runtime.attach_game_module(std::move(world_module)));
    assert(runtime.start_game());
    runtime.tick(1.0 / 60.0);
    assert(world_module_ptr->observed);
    assert(runtime.render_frame().size() == 1);
    assert(runtime.entity_alive(world_module_ptr->entity));
    const auto* world_transform = runtime.transform(world_module_ptr->entity);
    assert(world_transform != nullptr);
    assert(world_transform->position.z == 6.0f);
    runtime.stop_game();

    std::filesystem::remove_all(root);
    return 0;
}
