#include "gaming_runtime/runtime.h"

#include <cassert>

int main() {
    gaming_runtime::Runtime runtime({
        .target_fps = 60,
        .max_memory_mb = 512
    });

    gaming_runtime::GamePackage game{
        .id = "demo.game",
        .name = "Runtime Demo",
        .version = "0.1.0",
        .entry_point = "main",
        .estimated_memory_mb = 128
    };

    assert(runtime.load_game(game));
    assert(runtime.can_run_locally());

    runtime.tick(1.0 / 60.0);
    assert(runtime.frame_state().frame_number == 1);
    assert(runtime.frame_state().delta_seconds > 0.0);

    return 0;
}
