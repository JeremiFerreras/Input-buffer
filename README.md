# Geode Predictive Input Buffer

A Geometry Dash Geode mod that recreates the predictive input-buffer and live-console workflow shown in **“Top Players are cheating in Geometry Dash”**. It simulates short player trajectories, looks for safer input timing, and can delay eligible inputs so they reach the game on a later physics tick.

This is a community recreation / experimental fork, not an official Geometry Dash feature. The hidden simulator is separate from the live player; the mod does not move the real player to test a prediction.

## Features

- **Predictive input buffering:** checks whether an input made now is likely to survive, then searches for a safer delay inside the active stage's window.
- **Preserved input gestures:** queued presses and releases retain their order and spacing. If the queue reaches its hard limit, pending events are flushed in order and the new physical input is passed through instead of being swallowed.
- **Live prediction diagnostics:** compares predicted and real player positions after the click checkpoint, reports meaningful misses, and performs a low-frequency one-tick self-check when a clean comparison is available.
- **Moving-hazard prediction:** samples nearby hazards at a throttled rate and only forecasts a visible, axis-aligned mover after two consecutive position predictions have been validated. Otherwise Geometry Dash's normal collision check remains in use.
- **Automatic level scan:** starts on the first real physics step after entering a compatible level. It continues in small CPU slices while the level is playing; pausing is not required.
- **Separate Windows console:** status and useful diagnostics are written to a command-prompt console.

## Controls

| Key | Action |
|---|---|
| `F6` | Cycle Stage 1 → Stage 2 → Stage 3 → Stage 4 → Off |
| `F7` | Close / reopen the Windows live console |

The prediction stages use these base-tick windows:

| Stage | All inputs | Wave inputs |
|---|---:|---:|
| Stage 1 | 2 ticks | 2 ticks |
| Stage 2 | 4 ticks | 4 ticks |
| Stage 3 | 8 ticks | 8 ticks |
| Stage 4 | 16 ticks | 8 ticks |
| Off | No buffering | No buffering |

The stages are fixed profiles. Automatic/adaptive window calibration is not enabled in this fork.

## Automatic scan

The scan begins after Geometry Dash has initialized the attempt's real player state. It then explores trajectories using hidden simulation players, with a target budget of about **0.25 ms per rendered frame**. The real level keeps running normally.

The scanner currently skips dual and platformer sections. When it encounters a confidently tracked moving hazard, it temporarily increases beam width, uses finer height buckets, and tests input changes every tick for the nearby section. This spends the same fixed scan budget more carefully around dynamic obstacles; it does not raise the per-frame CPU budget. Rotating hazards, trigger-driven changes, unsupported mechanics, and complex portal interactions can still produce incomplete or approximate routes. **Use Scanned Route** remains experimental and is off by default; the scan never drives the real player, and a completed route is not a guarantee that every section is safe.

## Compatibility and limitations

The project is configured for Geode 5.11.2 and Geometry Dash 2.2081. The external command-prompt console is Windows-only. Prediction is best-effort: no delay can save every input, and game mods that change physics or intercept inputs can affect results.

The core unit tests exercise the scheduling and prediction helpers without launching Geometry Dash. They do not prove that every in-game collision or level mechanic is modelled perfectly.

## Build and tests

The GitHub Actions workflow builds the mod for Windows and separately compiles/runs the standalone core tests.

With a local Geode SDK installation:

```sh
geode build
```

To run the core tests directly with a C++23 compiler:

```sh
g++ -std=c++23 -O2 -Isrc tests/core_tests.cpp -o core_tests
./core_tests
```

## Reference

Workflow and presentation reference: **“Top Players are cheating in Geometry Dash”** (video supplied for this project).
