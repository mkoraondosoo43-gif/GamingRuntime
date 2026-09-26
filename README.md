# GamingRuntime

A standalone, local-first gaming runtime for Android.

## Current milestone: Runtime Core v0.3

GamingRuntime now has its own logical game library layer.

The storage system can:

- initialize a runtime-owned game library directory;
- recognize game directories containing `game.json`;
- discover installed games;
- return each game's directory and manifest path.

### Game library layout

```text
GamingRuntime/
└── games/
    └── demo_game/
        └── game.json
```

### Architecture

Game Package -> Runtime Storage -> Manifest -> Runtime Core -> Platform/Hardware Bridge

Android remains the underlying hardware/platform layer while GamingRuntime owns the game execution pipeline.

## Build

The core is portable C++17 and can later be integrated into an Android NDK/AOSP-based system.
