#include "gaming_runtime/storage.h"

#include <cassert>
#include <filesystem>
#include <fstream>

int main() {
    const std::filesystem::path root = "runtime_test_library";
    const std::filesystem::path game = root / "demo_game";

    std::filesystem::remove_all(root);
    std::filesystem::create_directories(game);

    {
        std::ofstream manifest(game / "game.json");
        manifest << R"({
            "id": "demo.game",
            "name": "Runtime Demo",
            "version": "0.3.0",
            "entry_point": "main",
            "estimated_memory_mb": 128
        })";
    }

    gaming_runtime::GameStorage storage(root.string());

    assert(storage.initialize());
    assert(storage.root_directory() == root.string());

    const auto games = storage.discover_games();
    assert(games.size() == 1);
    assert(games[0].manifest_path ==
           (game / "game.json").string());

    std::filesystem::remove_all(root);
    return 0;
}
