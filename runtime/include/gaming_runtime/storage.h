#pragma once

#include <string>
#include <vector>

namespace gaming_runtime {

struct StoredGame {
    std::string directory;
    std::string manifest_path;
};

class GameStorage {
public:
    explicit GameStorage(std::string root_directory);

    bool initialize();
    bool add_game_directory(const std::string& game_directory);
    std::vector<StoredGame> discover_games() const;

    const std::string& root_directory() const noexcept;

private:
    std::string root_directory_;
};

} // namespace gaming_runtime
