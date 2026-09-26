#include "gaming_runtime/runtime.h"

#include <cassert>
#include <fstream>

int main() {
    const char* manifest_path = "runtime_demo_manifest.json";

    {
        std::ofstream manifest(manifest_path);
        manifest << R"({
            "id": "demo.game",
            "name": "Runtime Demo",
            "version": "0.2.0",
            "entry_point": "main",
            "estimated_memory_mb": 128
        })";
    }

    gaming_runtime::Runtime runtime({
        .target_fps = 60,
        .max_memory_mb = 512
    });

    assert(runtime.load_manifest(manifest_path));
    assert(runtime.loaded_game().id == "demo.game");
    assert(runtime.loaded_game().version == "0.2.0");
    assert(runtime.can_run_locally());

    runtime.tick(1.0 / 60.0);
    assert(runtime.frame_state().frame_number == 1);
    assert(runtime.frame_state().delta_seconds > 0.0);

    std::remove(manifest_path);
    return 0;
}
