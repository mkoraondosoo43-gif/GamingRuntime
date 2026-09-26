#include "gaming_runtime/runtime.h"

#include <cassert>
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

    runtime.tick(1.0 / 60.0);
    assert(runtime.frame_state().frame_number == 1);

    std::filesystem::remove_all(root);
    return 0;
}
