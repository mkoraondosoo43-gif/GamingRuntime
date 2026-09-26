#pragma once

#include <cstdint>
#include <string>

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
    std::uint64_t estimated_memory_mb = 0;
};

struct FrameState {
    std::uint64_t frame_number = 0;
    double delta_seconds = 0.0;
};

class Runtime {
public:
    explicit Runtime(RuntimeConfig config = {});

    bool load_game(const GamePackage& package);
    bool load_manifest(const std::string& manifest_path);
    bool load_game_from_storage(const std::string& game_id,
                                const std::string& storage_root);

    bool can_run_locally() const;
    void tick(double delta_seconds);

    const FrameState& frame_state() const noexcept;
    const GamePackage& loaded_game() const noexcept;

private:
    RuntimeConfig config_;
    GamePackage game_;
    FrameState frame_;
    bool game_loaded_ = false;
};

} // namespace gaming_runtime
