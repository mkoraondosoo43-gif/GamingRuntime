#pragma once

#include <cstdint>

namespace gaming_runtime {

struct GameFrameContext {
    std::uint64_t frame_number = 0;
    double delta_seconds = 0.0;
};

class GameModule {
public:
    virtual ~GameModule() = default;

    virtual bool initialize() = 0;
    virtual void update(const GameFrameContext& context) = 0;
    virtual void shutdown() = 0;
};

} // namespace gaming_runtime
