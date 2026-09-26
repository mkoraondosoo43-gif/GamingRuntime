# GamingRuntime

A standalone, local-first gaming runtime for Android.

## Current milestone: Runtime Game Memory v1.0

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

### Package manifest

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

Invalid arithmetic stack usage or invalid register access halts the module instead of continuing with undefined state.

## Build

The core is portable C++17 and can later be integrated into an Android NDK/AOSP-based system.
