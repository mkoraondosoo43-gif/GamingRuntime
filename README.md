# GamingRuntime

A standalone, local-first gaming runtime for Android.

## Current milestone: Frame Budget Scheduler v1.9

GamingRuntime now lets each game package declare its runtime-owned bytecode program in `game.json`.

The bytecode VM can:

- load a text-based runtime bytecode program;
- execute stack operations;
- perform integer add, subtract, and multiply operations;
- write results into eight VM registers;
- halt safely on invalid stack/register operations;
- cap instructions executed per frame;
- cap the VM stack size;
- read and write VM registers;
- compare values with `EQ`;
- branch with `JMP` and `JZ`;
- persist game state in 256 runtime-owned memory slots;
- load/store game memory with `LOAD` and `STORE`;
- load and enumerate package assets through a runtime-owned asset manager;
- build bounded runtime render frames with clear and quad commands;
- submit render frames through a renderer interface;
- provide a NullRenderer for backend-independent testing;
- render actual RGBA pixels with the built-in SoftwareRenderer;
- enforce a 16-million-pixel framebuffer safety limit;
- build bounded runtime render frames with clear and quad commands;
- submit render frames through a renderer interface;
- provide a NullRenderer for backend-independent testing;
- safely read bounded asset files;
- reject asset paths that escape the package asset directory;
- run through the existing GameModule lifecycle.

### Game-control bytecode

The VM now supports the basic control flow needed for stateful game logic:

    PUSH 7
    SET 0
    GET 0
    PUSH 7
    EQ
    JZ 10
    PUSH 42
    SET 1
    HALT
    PUSH 99
    SET 1
    HALT

This lets game code make decisions without leaving the runtime VM. Jump targets are instruction indexes and invalid targets halt execution.

### Runtime-owned game memory

The VM now provides 256 persistent signed 64-bit game-memory slots. This memory belongs to the runtime VM and survives across frame updates while the module is running.

Example:

    PUSH 100
    STORE 5
    LOAD 5
    SET 0
    HALT

This stores `100` in memory slot 5, reads it back, and places it in register 0. Invalid memory indexes halt execution.


### Runtime memory budget

v1.6 adds a bounded runtime memory accounting layer. Each loaded game reserves its declared `estimated_memory_mb` against the runtime budget before the game becomes active. Oversized packages are rejected without replacing the current runtime state.

The memory manager tracks budget, used bytes, and available bytes. It is an accounting/safety boundary, not virtual RAM: it cannot create physical memory that the device does not have.



### Renderer memory budget

v1.8 connects renderer framebuffer allocation to the runtime memory budget. A renderer reports its framebuffer memory footprint, and Runtime::attach_renderer() reserves that footprint before making the renderer active. If the budget cannot support the framebuffer, attachment fails without changing the active renderer or runtime accounting. Replacing a renderer releases the old framebuffer budget before reserving the new footprint.

The built-in SoftwareRenderer reports its RGBA8 framebuffer size (width * height * 4) as its runtime memory footprint. This remains accounting of runtime-owned resources; it does not create or emulate physical RAM.

### Runtime resource memory

v1.7 connects the runtime memory budget to texture resources. Texture uploads reserve their RGBA8 byte size from the runtime memory budget, replacements adjust the accounting to the new size, and explicit texture unloading releases the tracked memory. A texture upload that would exceed the remaining budget is rejected before the renderer resource is committed.

Game reloads also tear down the previous renderer and clear tracked texture resources so resources from one game cannot remain attached to another game's runtime state.

### Runtime input layer

v1.5 adds a platform-neutral input boundary. The runtime owns the current input state and game modules receive a read-only snapshot through GameFrameContext.

The input layer supports:

- 64 runtime-owned digital buttons;
- 8 normalized analog axes in the range -1.0 to 1.0;
- normalized pointer/touch position;
- pointer/touch pressed state.

The runtime exposes setters such as:

    runtime.set_input_button(2, true);
    runtime.set_input_axis(0, 1.0f);
    runtime.set_pointer_input(120.0f, 80.0f, true);

A future Android input bridge can translate touch, gamepad, keyboard, or other platform events into this boundary without making game code depend directly on Android APIs.

### Software rendering backend

GamingRuntime now includes a deterministic CPU-based `SoftwareRenderer`. It consumes the same runtime render commands and produces an RGBA8 framebuffer. This makes the rendering path executable and testable without depending on Android, Vulkan, OpenGL ES, or a physical GPU.

The software backend now rasterizes clear, rectangle, and registered RGBA8 texture resources. It remains a development and compatibility foundation rather than the final hardware-accelerated Android backend.

The runtime now has a frame-budget scheduler. `RuntimeConfig::target_fps` defines the target frame budget, `Runtime::tick()` uses the scheduler to sanitize incoming delta time, and `FrameState` reports whether the supplied frame time exceeded the target budget. This is timing and telemetry groundwork; it does not sleep, overclock hardware, or create performance that the device does not have.


Framebuffer allocation is capped at 16 million pixels to keep accidental allocations bounded.


### Texture resources

v1.4 adds an end-to-end runtime texture path. Game assets are read by the runtime AssetManager, validated as raw RGBA8 data, uploaded through the Renderer boundary, and sampled by SoftwareRenderer when a draw-quad command references the resource ID.

The first runtime-native texture format is raw RGBA8: exactly width * height * 4 bytes, row-major, four bytes per pixel. PNG/JPEG decoding is intentionally not claimed yet; this keeps the resource pipeline real and deterministic before adding image decoders.

Example:

    runtime.load_texture_asset(12, "textures/checker.rgba", 2, 2);

This reads exactly 16 bytes for a 2x2 texture and uploads them under resource ID 12.

### Runtime rendering interface

GamingRuntime now owns a small rendering command layer. Games can build a bounded `RenderFrame` containing clear and quad commands, while the runtime submits that frame to a `Renderer` backend. `NullRenderer` provides a backend-independent implementation for testing.

The rendering interface deliberately does not pretend to be a GPU API yet. A future Android backend can translate these runtime commands to Vulkan, OpenGL ES, or another supported graphics API without changing the game-facing runtime boundary.

The default frame command limit is 4096 commands.

### Runtime rendering interface

GamingRuntime now owns a small rendering command layer. Games can build a bounded `RenderFrame` containing clear and quad commands, while the runtime submits that frame to a `Renderer` backend. `NullRenderer` provides a backend-independent implementation for testing.

The rendering interface deliberately does not pretend to be a GPU API yet. A future Android backend can translate these runtime commands to Vulkan, OpenGL ES, or another supported graphics API without changing the game-facing runtime boundary.

The default frame command limit is 4096 commands.

### Runtime asset management

GamingRuntime now treats a package's `assets/` directory as a runtime-owned resource space. The `AssetManager` can check for an asset, enumerate regular files, and read bounded binary data. Relative paths are normalized and rejected when they escape the asset root.

Example package resources:

    assets/
        textures/hero.bin
        audio/hit.bin

The runtime initializes the asset manager when a game package loads, so game execution and resource access share the same package boundary.

    {
        "format_version": 1,
        "id": "demo.game",
        "name": "Runtime Demo",
        "version": "0.8.0",
        "entry_point": "main",
        "bytecode": "game.bc",
        "assets": "assets",
        "estimated_memory_mb": 128
    }

The runtime validates the declared executable path, rejects paths outside the package root, and attaches the bytecode module automatically.

### Example runtime bytecode

    PUSH 20
    PUSH 22
    ADD
    SET 0
    HALT

This computes 42 and stores it in VM register 0.

### Execution architecture

    Game Package
          |
    Runtime Storage
          |
    Package Validation
          |
    Runtime Core
          |
    Game Execution Backend
          |
    Bytecode VM
          |
    GameModule
          |
    Platform / Hardware Bridge

The package now describes what executable backend the runtime should load instead of requiring the caller to manually provide the bytecode path.

The bytecode backend is intentionally small and portable. It is not being presented as a way to run arbitrary existing Windows, console, or Android executables. It establishes a genuine instruction-execution layer that GamingRuntime controls.

A future backend can translate or execute other supported game-code formats while keeping the same GameModule boundary.

### Safety boundaries

The VM currently limits:

- instructions per frame: 64;
- stack entries: 1024;
- registers: 8;
- game memory slots: 256;
- asset reads: 16 MiB default per read;
- runtime memory budget: configured in `RuntimeConfig::max_memory_mb`;

Invalid arithmetic stack usage or invalid register access halts the module instead of continuing with undefined state.

## Build

The core is portable C++17 and can later be integrated into an Android NDK/AOSP-based system. The renderer boundary is designed so platform graphics backends can be added separately.
