# GamingRuntime

A standalone, local-first gaming runtime for Android.

## Current milestone: Game Execution Boundary v0.6

GamingRuntime now has its first real game-execution boundary.

The runtime can:

- validate and load a game package;
- attach a runtime-owned `GameModule`;
- initialize the module before execution;
- provide frame number and delta time to the module;
- execute the module once per runtime frame;
- stop the module through an explicit shutdown lifecycle;
- reject execution when the package is not locally runnable.

### Package layout

    GamingRuntime/
    └── games/
        └── demo_game/
            ├── game.json
            └── assets/
                ├── textures/
                ├── models/
                ├── audio/
                └── ...

### Execution flow

    Game Package
          |
    Runtime Storage
          |
    Package Validation
          |
    Runtime Core
          |
    GameModule
      initialize()
          |
      update(frame)
          |
      update(frame)
          |
       shutdown()
          |
    Platform / Hardware Bridge

### Game module boundary

`GameModule` is the controlled interface between GamingRuntime and actual game logic.

It currently exposes:

- `initialize()`
- `update(GameFrameContext)`
- `shutdown()`

This is intentionally an execution boundary, not a fake claim that arbitrary Android, Windows, or console executables can already run.

The next execution layer can plug a real game-code backend into this boundary, such as a sandboxed bytecode/WASM-style module or a native translated module.

### Architecture

Game Package -> Runtime Storage -> Package Validation -> Runtime Core -> Game Execution Backend -> Graphics/Audio/Input/Memory Systems -> Platform/Hardware Bridge

Android remains the underlying hardware/platform layer while GamingRuntime owns the game execution pipeline.

## Build

The core is portable C++17 and can later be integrated into an Android NDK/AOSP-based system.
