# Changelog

## v4.7.0

### Input feedback and console
- Added a clear `PERFECT` decision for inputs predicted safe at their original timing.
- Expanded `SAVED +N TICKS` messages with gameplay mode, input type, original and delayed frame positions, delay-search count, simulation time, and the selected limit.
- Updated the startup banner to report the current mod version.

### Prediction and scanning
- Enabled moving-hazard tracking across gameplay modes.
- Throttled motion sampling to avoid repeating full object-list work on every Click Between Frames sub-step.
- Require two consecutive successful motion validations before using a moving-hazard forecast.
- Keep normal Geometry Dash collision handling for hazards whose movement cannot be trusted.
- Increase scanner resolution locally around a confirmed moving hazard without increasing the scan's per-frame CPU budget.
- Report the number of moving-hazard predictions used by the scanner.

### Reliability and project presentation
- Retained the input-queue overflow fallback: pending events are flushed in order and the new physical input is passed through.
- Updated the project manifest, CMake project identity, installation instructions, settings descriptions, support information, and documentation.
- Clarified that this is input assistance, not an auto-player: it never generates clicks or controls the real player.

## Earlier engine history

The codebase builds on the Input Buffer trajectory-simulation and input-scheduling engine by AlQuerez23. Earlier engine-specific implementation notes remain in the source history.
