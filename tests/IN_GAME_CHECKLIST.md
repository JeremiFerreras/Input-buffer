# In-game verification checklist

Use this checklist when validating a new Windows package in Geometry Dash 2.2081.

- [ ] Startup creates a separate Windows console titled `Geode Predictive Input Buffer - Live Console`.
- [ ] Startup output reports version v4.7.0, the selected stage, and the F6 cycle.
- [ ] F6 cycles Stage 1 → Stage 2 → Stage 3 → Stage 4 → Off → Stage 1, and each transition is printed.
- [ ] An input predicted safe at its original timing is passed through and logged as `PERFECT` with zero delay.
- [ ] An input delayed by prediction produces a `SAVED +N TICKS` line with mode, input type, original/delayed frames, tested candidates, simulation time, and limit.
- [ ] If no safe timing is found, or prediction cannot continue safely, the input falls back to the game and the console reports the relevant `RAW` reason.
- [ ] Double taps and press/release timing remain ordered when an earlier input is buffered.
- [ ] Ship and wave input gestures remain consistent at 60, 144, 240, and high-TPS / Click Between Frames configurations.
- [ ] Death, respawn, practice checkpoints, and level exit clear pending buffered inputs.
- [ ] Automatic scanning starts after the first real physics step of a compatible attempt while the level continues to play.
- [ ] Scan progress is reported in the console and does not create path drawings or take control of the real player.
- [ ] Test supported moving hazards: a forecast is used only after two consecutive motion validations; untrusted or unsupported hazards use normal game collision handling.
- [ ] Test portal-, orb-, and pad-heavy areas; unsupported predictions must fall back to raw input instead of claiming a successful save.
- [ ] F7 closes and reopens the console without closing Geometry Dash.
- [ ] The optional Use Scanned Route setting stays off by default and only aligns physical inputs when explicitly enabled.

## Scope

The standalone C++ core tests validate helper logic, not every live Geometry Dash collision. Record any difference between the console's prediction and the actual in-game result.
