# Micro-cancel real-game replay QA

This is the controlled Phase 2 gate before the later adversarial stress campaign.

## Why this method

The replay harness is embedded in the Geode mod and uses the active Geometry Dash 2.2081 engine. It captures the opened level string, makes a fresh local `PlayLayer` scene for each job, injects the same press/release tape at the beginning of the real `GJBaseGameLayer::processCommands` call, and logs state after the engine processes the step.

This follows the useful part of established GD replay tooling: replay a deterministic input tape inside the game and compare per-step traces. It avoids treating an approximate standalone Wave/Ship implementation as ground truth. The hidden `PlayerObject` simulator still authorizes Micro-cancel in real play; this replay harness measures the actual outcome of each profile.

## File format

Create `micro-cancel-qa/jobs.txt` in the Geode save directory. The mod logs its exact expected path once when a level opens.

Each job has:
```text
job <job_name> <group_name> <profile> <max_steps>
inputs <step>:<state> <step>:<state> ...
```

- `profile`: 0 = Micro-cancel OFF, 1 = 1T, 2 = 2T, 3 = 3T. The normal Input Buffer stays enabled and its F6 stage remains the same across jobs.
- `step`: zero-based invocation count of `processCommands`; this is not wall-clock frames.
- `state`: 1 = press, 0 = release.
- Every tape must start with a press, alternate press/release, use increasing step numbers, and end with a release.
- A group must contain profile 0 and at least one active profile. Every job in a group must carry identical event steps.
- Use a fresh job name for every profile. The harness enforces unique names and profile entries per group.

An illustrative Wave tape is in [micro_cancel_qa.example.txt](micro_cancel_qa.example.txt). Replace its steps with the exact sequence needed for your local test level. Use a separate local level/batch for a Ship segment that starts differently.

## What the harness records

Per-step CSV: position, vertical velocity, gamemode, gravity direction, size, speed, ground/dead state, physical/game button state, latch state, last Input Buffer decision, candidate plan ticks, synthetic releases, swallowed physical key-ups, cancellations, and expected-versus-actual next press timing.

Final results:
- `SAVED`: OFF died, active profile survived through level completion or the configured `max_steps` horizon.
- `REGRESSION`: OFF survived, active profile died.
- `DIED_EARLIER` / `DIED_LATER` / `DIED_BOTH`: both died; the active profile did not save the run.
- `NO_DEATH_BOTH`: neither run died within the tested horizon.
- `_NO_INTERVENTION`: the profile did not send a synthetic release in that job.
- `CHECK_LATCH`: a pending/released latch or inconsistent event counters remained at the end.

Files are written to `micro-cancel-qa/out/`: `results.csv`, `comparison.csv`, and one trace CSV per job. The source `jobs.txt` is renamed to `jobs.done.txt` when complete, `jobs.aborted.txt` on pause/quit, or `jobs.invalid.txt` when validation fails.

## Limits

This is real Geometry Dash 2.2081 physics and collision processing, with the button API invoked at the same command boundary for each comparison. It is deterministic and comparable, but it does not measure physical keyboard latency, operating-system scheduling, or a human reaction. A timeout means the player survived the configured horizon, not necessarily that the whole level was completed. Set `max_steps` beyond the complete section under test.

Do not begin the large stress campaign until there are no unexplained `REGRESSION`, `CHECK_LATCH`, or `DIED_EARLIER` outcomes.
