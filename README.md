# GamingRuntime

A standalone, local-first gaming runtime for Android.

## Current milestone: Bytecode Execution Backend v0.7

GamingRuntime now contains a real runtime-owned bytecode execution backend.

The bytecode VM can:

- load a text-based runtime bytecode program;
- execute stack operations;
- perform integer add, subtract, and multiply operations;
- write results into eight VM registers;
- halt safely on invalid stack/register operations;
- cap instructions executed per frame;
- cap the VM stack size;
- run through the existing GameModule lifecycle.

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

The bytecode backend is intentionally small and portable. It is not being presented as a way to run arbitrary existing Windows, console, or Android executables. It establishes a genuine instruction-execution layer that GamingRuntime controls.

A future backend can translate or execute other supported game-code formats while keeping the same GameModule boundary.

### Safety boundaries

The VM currently limits:

- instructions per frame: 64;
- stack entries: 1024;
- registers: 8.

Invalid arithmetic stack usage or invalid register access halts the module instead of continuing with undefined state.

## Build

The core is portable C++17 and can later be integrated into an Android NDK/AOSP-based system.
