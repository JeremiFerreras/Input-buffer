# Geode Predictive Input Buffer

[![Build](https://github.com/JeremiFerreras/Input-buffer/actions/workflows/build.yml/badge.svg)](https://github.com/JeremiFerreras/Input-buffer/actions/workflows/build.yml)

**Input-timing assistance for Geometry Dash.** Predictive Input Buffer checks the trajectory that follows an input and, when necessary, delays that input to the earliest safer physics tick inside your selected timing window.

The goal is to make difficult timings more forgiving while keeping the player in control. This is an assist mod, **not an auto-player or level bot**: it does not invent clicks, steer the icon, or complete sections for you. It only works with inputs you physically make.

## What it does

- **Predicts before passing an input.** Hidden simulator players test short trajectories using the current player state. If the input is already safe, it passes through unchanged.
- **Finds the earliest safer tick.** If the immediate trajectory is predicted to die, the mod searches the active stage's allowed delay window and buffers the input only when it finds a usable timing.
- **Preserves your gesture.** Presses and releases remain ordered, with their spacing maintained while a buffered input is waiting.
- **Checks moving hazards conservatively.** A visible, axis-aligned hazard is predicted at its future position only after its movement model has passed consecutive accuracy checks. Otherwise the game keeps its normal collision handling.
- **Scans compatible levels.** A hidden beam scanner explores possible input routes in small CPU slices while the level continues to play. It never takes control of the real player.
- **Shows its decisions.** The separate Windows console reports stage changes, scan progress, safe inputs, buffered inputs, and prediction diagnostics.

## Install

1. Install [Geode](https://geode-sdk.org/) for Geometry Dash 2.2081.
2. Open the repository's [GitHub Actions builds](https://github.com/JeremiFerreras/Input-buffer/actions/workflows/build.yml).
3. Download the **Mod Windows** artifact from the latest successful run and extract the ZIP.
4. Install the included `.geode` file with Geode, or place it in the game's `geode/mods` folder.
5. Launch Geometry Dash. On Windows, the live console opens with the mod.

The workflow currently produces a Windows build. The external command-prompt console is Windows-only.

## Controls

| Key | Action |
|---|---|
| `F6` | Cycle the prediction stage |
| `F7` | Close or reopen the live console |

The stage cycle is:

`Stage 1 → Stage 2 → Stage 3 → Stage 4 → Off → Stage 1`

You can change the keybinds in the mod's Geode settings.

## Prediction stages

The stage is the maximum delay the predictor may use. It does not force every input to wait: an input that is already safe passes through immediately.

| Stage | All modes | Wave |
|---|---:|---:|
| Stage 1 | 2 ticks | 2 ticks |
| Stage 2 | 4 ticks | 4 ticks |
| Stage 3 | 8 ticks | 8 ticks |
| Stage 4 | 16 ticks | 8 ticks |
| Off | No buffering | No buffering |

A base tick is approximately 1/240 of a second at the default 240-TPS physics rate. Four base ticks are approximately one 60-FPS frame. The engine uses the game's detected physics timing, so the real-time duration follows the active timing configuration.

Larger stages give the predictor more room to find a safe moment, but can make an assisted input feel later. Start with Stage 1 and increase the window only when you need more room.

## Reading the console

The console uses short decision labels:

- **PERFECT** — the current input is predicted to be safe, so it passes through without a delay.
- **SAVED +N TICKS** — the current timing is predicted to be unsafe and the input is scheduled N physics ticks later. The line also reports the mode, original and delayed frame positions, number of tested delay candidates, simulation time, and active limit.
- **RAW** — the predictor could not safely make a timing change, for example because the path reaches an unsupported mechanic, the delay window has no solution, or the simulation budget is exhausted.

A `SAVED` message is the simulator's prediction and scheduling decision, not a guarantee against every possible in-game collision. The real game state and supported mechanics still determine what happens.

## Automatic level scan

Automatic Level Scan is enabled by default. It starts after the first real physics step of a compatible attempt and explores the level using hidden simulation players. It continues in small slices while you play, with a target budget of about **0.25 ms per rendered frame**.

When a confidently tracked moving hazard is encountered, the scanner temporarily increases its search width, uses finer height buckets, and tests possible input changes every tick around that area. The per-frame scan budget stays the same.

The scan currently does not start in dual or platformer gameplay. Trigger-driven changes, rotating hazards, unsupported objects, and complex portal interactions can still make a scanned route incomplete or approximate.

### Use Scanned Route

The **Use Scanned Route** setting is off by default. When enabled, an early physical input can be aligned with a matching input point from the scanned route. This feature only adjusts an input you make; it does not create inputs or drive the player. Because route data can be incomplete on unsupported sections, keep it off unless you have checked the scan for that level.

## Settings

| Setting | Default | Purpose |
|---|---|---|
| Prediction Stage | Stage 1 | Selects the maximum delay window |
| Stage Cycle Key | F6 | Changes the active stage |
| Console Toggle Key | F7 | Shows or hides the Windows console |
| Automatic Level Scan | On | Scans supported levels on attempt entry |
| Use Scanned Route | Off | Optionally aligns early manual inputs to a scanned route |

## Compatibility and limitations

- Configured for **Geometry Dash 2.2081** and **Geode 5.11.2**.
- The GitHub Actions package currently builds and verifies Windows.
- The separate command-prompt console is Windows-only.
- The predictor supports many common gameplay mechanics, but not every object, trigger, portal sequence, or third-party physics modification.
- Moving-hazard forecasts are limited to visible, axis-aligned hazards with a sufficiently validated motion model. Untrusted movement is left to Geometry Dash's normal collision handling.
- Dual and platformer gameplay are not supported by the automatic scan.
- No delay can save an input when no safe timing exists inside the active stage window.

Mods that alter physics, TPS, or input dispatch can affect predictions. For leaderboard or submitted runs, check the rules of the relevant list or event before using any input-assistance mod.

## Build from source

Requirements: Geode SDK 5.11.2, Geometry Dash 2.2081 bindings, CMake, and a C++23 compiler.

```sh
geode build
```

Run the standalone core tests with a C++23 compiler:

```sh
g++ -std=c++23 -O2 -Isrc tests/core_tests.cpp -o core_tests
./core_tests
```

The core tests cover scheduling and prediction helpers. They do not simulate every Geometry Dash object or replace in-game verification.

## Credits

The trajectory-simulation and input-scheduling engine builds on the **Input Buffer** project by **AlQuerez23**. This project adds its own timing-stage configuration, console presentation, diagnostics, scanning behavior, and maintenance.

Predictive Input Buffer is an independent community mod and is not affiliated with or endorsed by RobTop Games.
