# Geode Predictive Input Buffer

**Predictive Input Buffer** helps Geometry Dash players with difficult input timings. It predicts short player trajectories and can move an eligible physical input to a safer tick within the selected timing window.

The mod keeps the player in control:

- Inputs predicted safe are passed through immediately.
- Unsafe inputs are delayed only when a safer timing is found inside the active stage.
- Presses and releases keep their original order.
- The mod never invents clicks, steers the icon, or completes a level automatically.

## Quick controls

| Key | Action |
|---|---|
| F6 | Cycle Stage 1 → Stage 2 → Stage 3 → Stage 4 → Off |
| F7 | Close or reopen the Windows live console |

## Credits

The trajectory-simulation and input-scheduling engine builds on Input Buffer by AlQuerez23. Predictive Input Buffer is an independent community project, not an official Geometry Dash feature.
