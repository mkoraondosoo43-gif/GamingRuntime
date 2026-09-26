#include "gaming_runtime/runtime.h"

#include <cassert>
#include <memory>
#include <filesystem>
#include <fstream>

int main() {
    const std::filesystem::path root = "runtime_test_library";
    const std::filesystem::path game = root / "demo_game";
    const std::filesystem::path assets = game / "assets";

    std::filesystem::remove_all(root);
    std::filesystem::create_directories(assets);

    {
        std::ofstream manifest(game / "game.json");
        manifest << R"({
            "format_version": 1,
            "id": "demo.game",
            "name": "Runtime Demo",
            "version": "0.5.0",
            "entry_point": "main",
            "assets": "assets",
            "estimated_memory_mb": 128
        })";
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
    assert(runtime.loaded_game().version == "0.5.0");
    assert(runtime.loaded_game().root_directory ==
           game.lexically_normal().string());
    assert(runtime.loaded_game().assets_directory ==
           assets.lexically_normal().string());
    assert(runtime.can_run_locally());

    {
        std::ofstream invalid_manifest(game / "game.json");
        invalid_manifest << R"({
            "format_version": 1,
            "id": "demo.game",
            "name": "Runtime Demo",
            "version": "0.5.0",
            "entry_point": "main",
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

    std::filesystem::remove_all(root);
    return 0;
}
