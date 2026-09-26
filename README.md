# GamingRuntime

A standalone, local-first gaming runtime for Android.

## First milestone

The first milestone is a small runtime core that can:

- load a game package from the runtime's own storage area;
- validate its manifest;
- create a runtime session;
- execute a deterministic game tick loop;
- expose basic input and frame state;
- report whether a package can run locally.

This is the foundation. Graphics translation, CPU instruction translation, memory virtualization, audio, Android hardware bridges, and compatibility layers will be added incrementally.

## Architecture

Game package -> Runtime Core -> Platform/Hardware Bridge

Android remains the underlying hardware/platform layer while the GamingRuntime owns the game execution pipeline.

## Build

The initial core is written in portable C++17 so it can later be integrated into an Android NDK/AOSP-based system.
