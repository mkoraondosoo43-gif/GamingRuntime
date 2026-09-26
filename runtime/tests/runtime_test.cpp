#include "gaming_runtime/runtime.h"
#include "gaming_runtime/bytecode.h"
#include "gaming_runtime/asset_manager.h"
#include "gaming_runtime/render.h"

#include <cassert>
#include <memory>
#include <filesystem>
#include <fstream>
#include <vector>

int main() {
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
           game.lexically_normal().string());
    assert(runtime.loaded_game().assets_directory ==
           assets.lexically_normal().string());
    assert(runtime.can_run_locally());
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
    assert(runtime.assets().list_assets().size() == 3);

    auto renderer = std::make_unique<gaming_runtime::NullRenderer>();
    auto* renderer_ptr = renderer.get();
    assert(runtime.attach_renderer(std::move(renderer), 1280, 720));
    assert(runtime.render_frame().size() == 0);
    assert(runtime.render_frame().clear({0.02f, 0.03f, 0.05f, 1.0f}));
    assert(runtime.render_frame().draw_quad(10.0f, 20.0f, 100.0f, 50.0f, 7));
    assert(runtime.render_frame().size() == 2);

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

    class TestGame final : public gaming_runtime::GameModule {
    public:
        explicit TestGame(gaming_runtime::RenderFrame& frame) : frame(frame) {}

        bool initialize() override {
            initialized = true;
            return true;
        }

        void update(const gaming_runtime::GameFrameContext& context) override {
            frame.clear({0.0f, 0.0f, 0.0f, 1.0f});
            frame.draw_quad(2.0f, 3.0f, 10.0f, 8.0f, 7);
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
    };

    auto module = std::make_unique<TestGame>(runtime.render_frame());
    auto* module_ptr = module.get();

    assert(runtime.attach_game_module(std::move(module)));
    assert(runtime.start_game());
    assert(runtime.game_started());
    assert(module_ptr->initialized);

    runtime.tick(1.0 / 60.0);
    assert(runtime.frame_state().frame_number == 1);
    assert(renderer_ptr->submitted_frames() == 1);
    assert(renderer_ptr->last_command_count() == 2);
    assert(module_ptr->last_frame == 1);
    assert(module_ptr->last_delta > 0.0);

    runtime.stop_game();
    assert(!runtime.game_started());
    assert(module_ptr->shutdown_called);

    {
        auto software = std::make_unique<gaming_runtime::SoftwareRenderer>();
        auto* software_ptr = software.get();
        assert(runtime.attach_renderer(std::move(software), 8, 8));
        assert(runtime.load_texture_asset(12, "textures/checker.rgba", 2, 2));
        assert(software_ptr->has_texture(12));

        assert(runtime.render_frame().clear({0.0f, 0.0f, 0.0f, 1.0f}));
        assert(runtime.render_frame().draw_quad(0.0f, 0.0f, 4.0f, 4.0f, 12));
        assert(software_ptr->submit(runtime.render_frame()));

        const auto& pixels = software_ptr->pixels();
        assert(pixels[0] == 255);
        assert(pixels[1] == 0);
        assert(pixels[2] == 0);
        assert(pixels[4 * 4] == 0);
        assert(pixels[4 * 4 + 1] == 255);
        assert(pixels[4 * 4 + 2] == 0);
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

    assert(runtime.load_bytecode_module(bytecode.string()));
    assert(runtime.start_game());
    runtime.tick(1.0 / 60.0);
    runtime.stop_game();

    std::filesystem::remove_all(root);
    return 0;
}
