# Geode Predictive Input Buffer

**Experimental Geometry Dash / Geode recreation** of the predictive input-buffer and live-console workflow shown in the supplied video, “Top Players are cheating in Geometry Dash”.

## Overview

The mod simulates short player trajectories to estimate whether an input made now is safe. When a safer timing is found inside the current stage window, eligible inputs are buffered and replayed on a later physics tick. Presses and releases are kept in order so the original gesture is preserved.

A separate Windows console displays stage changes, scan progress, and useful prediction diagnostics. The simulator uses hidden player objects; it does not move the real player to test candidate trajectories.

## Features

- Predictive trajectory checks and bounded delay search.
- Queue safety fallback: if the event queue reaches capacity, pending events are flushed in order and the new physical input is allowed through.
- Prediction accuracy checks against the real player, plus a low-frequency one-tick self-check.
- Automatic level scan from the first real physics step, continuing in small CPU slices while the level plays.
- Windows command-prompt console, toggled with F7.
- Stage cycle on F6: Stage 1 → Stage 2 → Stage 3 → Stage 4 → Off.

## Scan limitations

The scan does not require pausing. It uses a snapshot of the player's initial live state and hidden simulation players, targeting about 1 ms of processing per rendered frame. Dual and platformer sections are skipped. Moving objects, trigger-driven changes, and unsupported mechanics can make a route approximate or incomplete. Route snapping is experimental and disabled by default.

## Prediction profiles

| Stage | All inputs | Wave inputs |
|---|---:|---:|
| Stage 1 | 2 ticks | 2 ticks |
| Stage 2 | 4 ticks | 4 ticks |
| Stage 3 | 8 ticks | 8 ticks |
| Stage 4 | 16 ticks | 8 ticks |
| Off | No buffering | No buffering |

Adaptive calibration is intentionally not enabled. Actual behavior still depends on the detected physics timing and mechanics supported by the simulator.

## Build and validation

GitHub Actions builds the Windows Geode mod and runs the standalone C++23 core tests. Passing those tests validates the covered helper logic only; it does not guarantee perfect collision prediction in every live level.
