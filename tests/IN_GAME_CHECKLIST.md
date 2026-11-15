# In-game validation checklist (video profile)

Run these tests in Geometry Dash 2.2081 after the GitHub Actions package builds.

- [ ] Startup creates a separate Windows console titled `Geode Predictive Input Buffer - Live Console`.
- [ ] Startup output includes version, selected mode, and the F6 stage sequence.
- [ ] No Input Buffer window, pause-menu button, player badge, or path overlay appears.
- [ ] F6 cycles Stage 1 -> Stage 2 -> Stage 3 -> Stage 4 -> Off -> Stage 1; each transition is printed to the console.
- [ ] A press/release saved by prediction produces a `[InputBuffer] SAVED +N TICKS (.. frames @ 60fps) PRESS! (Frame: .. -> Delayed (..)) [Limit: ..]` line.
- [ ] A click already predicted safe is passed through unchanged (no artificial press).
- [ ] Double taps and release timing remain ordered when a prior input is buffered.
- [ ] Ship and wave holds/releases are tested at 60, 144, 240 and high-TPS/CBF settings.
- [ ] Death, respawn, practice checkpoints, and level exit clear pending buffered inputs.
- [ ] Pausing a supported non-dual, non-platformer level starts the automatic scan and logs progress every 10%.
- [ ] Scan stops/finishes without spawning path drawings or the original Input Buffer overlay.
- [ ] Test portal/orb/pad-heavy levels; unsupported predictions must fall back to raw input, not claim a save.
- [ ] F7 closes and reopens the console without closing Geometry Dash.

## Important

Passing the standalone C++ unit tests is not proof of exact in-game prediction. Record any mismatch between console output and actual player behavior. A scan is advisory unless `Use Scanned Route (Experimental)` is enabled.


## Micro-cancel — Phase 2 focused validation

Run in Practice Mode with F9 OFF first, then compare the same section with F9 set to 1T, 2T and 3T. Keep the normal Input Buffer settings identical between runs.

- [ ] Confirm micro-cancel starts OFF after every game launch.
- [ ] Wave spam: test short and irregular taps, rising paths, descending paths, and both gravity directions.
- [ ] Wave: test a press where the normal predictor reports no safe delayed press; confirm the synthetic release occurs only on its selected physics tick.
- [ ] Confirm the player's next real press/release cadence is not replaced by an automatic input.
- [ ] Confirm the real physical key-up after a synthetic release is consumed exactly once and the next physical press reaches GD.
- [ ] Confirm the learned tap and gap timing is measured from the physical key-up, not the earlier synthetic release.
- [ ] Ship: test a stable straight segment while rising, level, and falling; include several initial vertical velocities and both normal/mini Ship.
- [ ] Confirm a mode-changing portal resets straight-flight evidence and cancels a still-pending incompatible release.
- [ ] Confirm no intervention occurs when the next normal tap would end before the scheduled release, or when only one follow-up timing survives.
- [ ] Confirm unknown geometry, moving hazards, orbs, dual/two-player control, queue activity, and budget exhaustion fail closed.
- [ ] Confirm an overdue physics tick is not converted into an immediate release.
- [ ] Confirm death, respawn, pause/resume, disabling F9, and changing gamemode do not leave a stale latch.
- [ ] Compare with F9 OFF: ordinary Input Buffer decisions, queue order, and press/release timing must remain unchanged.
- [ ] Repeat at 60, 120, 144, 170 and 240 FPS, with Click Between Frames both enabled and disabled, where supported.

Record the level, x-position, mode, speed portal, gravity, player size, FPS/TPS, F9 profile, predicted release tick, actual release tick, physical follow-up timing, and outcome. A successful synthetic test is not a substitute for these in-game checks.
