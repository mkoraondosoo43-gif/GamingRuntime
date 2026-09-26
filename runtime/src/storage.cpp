#include "gaming_runtime/storage.h"

#include <filesystem>

namespace gaming_runtime {
namespace fs = std::filesystem;

GameStorage::GameStorage(std::string root_directory)
    : root_directory_(std::move(root_directory)) {}

bool GameStorage::initialize() {
    std::error_code error;
    fs::create_directories(root_directory_, error);
    return !error;
}

bool GameStorage::add_game_directory(const std::string& game_directory) {
    if (!fs::exists(game_directory) || !fs::is_directory(game_directory)) {
        return false;
    }

    const fs::path manifest = fs::path(game_directory) / "game.json";
    return fs::exists(manifest) && fs::is_regular_file(manifest);
}

std::vector<StoredGame> GameStorage::discover_games() const {
    std::vector<StoredGame> games;

    if (!fs::exists(root_directory_) || !fs::is_directory(root_directory_)) {
        return games;
    }

    std::error_code error;
    for (const auto& entry : fs::directory_iterator(root_directory_, error)) {
        if (error) {
            break;
        }

        if (!entry.is_directory()) {
            continue;
        }

        const fs::path manifest = entry.path() / "game.json";
        if (fs::is_regular_file(manifest)) {
            games.push_back({
                entry.path().string(),
                manifest.string()
            });
        }
    }

    return games;
}

const std::string& GameStorage::root_directory() const noexcept {
    return root_directory_;
}

} // namespace gaming_runtime
