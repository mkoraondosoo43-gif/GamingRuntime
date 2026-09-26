# GamingRuntime

A standalone, local-first gaming runtime for Android.

## Current milestone: Runtime Core v0.2

The runtime can now load a game manifest from disk, validate the required package fields, create a runtime game session, and run the deterministic frame tick.

### Manifest

A game package currently describes:

- `id`
- `name`
- `version`
- `entry_point`
- `estimated_memory_mb`

Example:

```json
{
  "id": "demo.game",
  "name": "Runtime Demo",
  "version": "0.2.0",
  "entry_point": "main",
  "estimated_memory_mb": 128
}
```

## Architecture

Game package -> Manifest -> Runtime Core -> Platform/Hardware Bridge

Android remains the underlying hardware/platform layer while GamingRuntime owns the game execution pipeline.

## Build

The initial core is written in portable C++17 so it can later be integrated into an Android NDK/AOSP-based system.
