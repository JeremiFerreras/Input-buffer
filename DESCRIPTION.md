# Geode Predictive Input Buffer

Predictive Input Buffer is a Geometry Dash input-assistance mod. It evaluates short player trajectories when you press or release, passes safe inputs through immediately, and searches for the earliest safer physics tick when the current timing is predicted to be unsafe.

The mod is designed to assist the player without automating gameplay. It does not create clicks, steer the icon, or complete levels. Buffered presses and releases keep their order and timing relationship.

## Main features

- Predictive input timing with five selectable stages: Stage 1–4 and Off.
- A bounded delay search that respects the selected stage window.
- Ordered press/release scheduling and a safe queue-overflow fallback.
- Conservative moving-hazard prediction, enabled only for validated motion.
- Automatic hidden scanning on supported levels, using small per-frame CPU slices.
- A separate Windows live console with `PERFECT`, `SAVED +N TICKS`, and `RAW` decisions.

## Controls

| Key | Action |
|---|---|
| `F6` | Cycle Stage 1 → Stage 2 → Stage 3 → Stage 4 → Off |
| `F7` | Close or reopen the Windows live console |

## Timing windows

| Stage | All modes | Wave |
|---|---:|---:|
| Stage 1 | 2 ticks | 2 ticks |
| Stage 2 | 4 ticks | 4 ticks |
| Stage 3 | 8 ticks | 8 ticks |
| Stage 4 | 16 ticks | 8 ticks |
| Off | No buffering | No buffering |

A base tick is approximately 1/240 second at the default physics rate. The selected stage is a maximum delay, not a forced delay: inputs predicted safe at their original timing pass through unchanged.

## Automatic scanning

The scanner starts after the first real physics step of a compatible attempt and runs in small slices while the level plays. It uses hidden simulation players and never controls the real player. Dual and platformer gameplay are not supported by the automatic scan. Trigger-driven changes, rotating hazards, and unsupported mechanics may make a scanned route incomplete.

**Use Scanned Route** is optional and off by default. It can align an early physical input with a matching scanned point, but it never generates an input on its own.

## Compatibility

Configured for Geometry Dash 2.2081 and Geode 5.11.2. The repository's current build workflow produces a Windows package. The live command-prompt console is Windows-only.

## Source and credits

The trajectory-simulation and input-scheduling engine builds on Input Buffer by AlQuerez23. This repository maintains its own configuration, console interface, diagnostics, and scanning improvements.
