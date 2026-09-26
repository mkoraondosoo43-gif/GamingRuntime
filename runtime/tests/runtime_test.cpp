#include "gaming_runtime/runtime.h"
#include "gaming_runtime/bytecode.h"
#include "gaming_runtime/asset_manager.h"

#include <cassert>
#include <memory>
#include <filesystem>
#include <fstream>

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

    gaming_runtime::Runtime runtime({
        .target_fps = 60,
        .max_memory_mb = 512,
        .supported_package_format = 1
    });

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
    assert(runtime.assets().has_asset("textures/hero.bin"));
    assert(!runtime.assets().has_asset("../game.json"));
    std::vector<std::uint8_t> asset_data;
    assert(runtime.assets().read_asset("audio/hit.bin", asset_data));
    assert(asset_data.size() == 3);
    assert(runtime.assets().list_assets().size() == 2);

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

    class TestGame final : public gaming_runtime::GameModule {
    public:
        bool initialize() override {
            initialized = true;
            return true;
        }

        void update(const gaming_runtime::GameFrameContext& context) override {
            last_frame = context.frame_number;
            last_delta = context.delta_seconds;
        }

        void shutdown() override {
            shutdown_called = true;
        }

        bool initialized = false;
        bool shutdown_called = false;
        std::uint64_t last_frame = 0;
        double last_delta = 0.0;
    };

    auto module = std::make_unique<TestGame>();
    auto* module_ptr = module.get();

    assert(runtime.attach_game_module(std::move(module)));
    assert(runtime.start_game());
    assert(runtime.game_started());
    assert(module_ptr->initialized);

    runtime.tick(1.0 / 60.0);
    assert(runtime.frame_state().frame_number == 1);
    assert(module_ptr->last_frame == 1);
    assert(module_ptr->last_delta > 0.0);

    runtime.stop_game();
    assert(!runtime.game_started());
    assert(module_ptr->shutdown_called);

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
        module.update({.frame_number = 1, .delta_seconds = 1.0 / 60.0});
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
        module.update({.frame_number = 1, .delta_seconds = 1.0 / 60.0});
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
