#pragma once

#include <cstdint>
#include <string>

namespace gaming_runtime {

struct RuntimeConfig {
    std::uint32_t target_fps = 60;
    std::uint32_t max_memory_mb = 1024;
};

struct GamePackage {
    std::string id;
    std::string name;
    std::string version;
    std::string entry_point;
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
