# GamingRuntime

A standalone, local-first gaming runtime for Android.

## Current milestone: Package Format v0.5

GamingRuntime now understands a validated runtime-owned game package.

The package layer can:

- require a supported package format version;
- identify a game with id, name, and version;
- define an entry point for the future execution layer;
- define a package-owned assets directory;
- record the package root and resolved assets path;
- reject packages with missing required fields or missing asset directories;
- keep local execution checks separate from the future game-code execution engine.

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

### Manifest example

    {
        "format_version": 1,
        "id": "demo.game",
        "name": "Runtime Demo",
        "version": "0.5.0",
        "entry_point": "main",
        "assets": "assets",
        "estimated_memory_mb": 128
    }

### Architecture

Game Package -> Runtime Storage -> Package Validation -> Runtime Core -> Future Execution Layer -> Platform/Hardware Bridge

Android remains the underlying hardware/platform layer while GamingRuntime owns the game execution pipeline.

The entry_point is currently a package boundary only. Actual game-code execution will be implemented in a later milestone rather than being faked by the package loader.

## Build

The core is portable C++17 and can later be integrated into an Android NDK/AOSP-based system.
