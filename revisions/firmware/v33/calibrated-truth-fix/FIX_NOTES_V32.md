# NearSens v32 2 MHz calibration no-4006 emergency fix

Base: v31/v23 logic.

Changes:

- EEPROM_VERSION = 17 to force fresh calibration.
- CAL_POINT_SAMPLES = 9, CAL_POINT_MIN_GOOD = 1, CAL_POINT_MAX_SPREAD_MM = 30.
- `captureCalibrationPoint` no longer fails with 4006 just because one packet is an outlier.
- It sorts good packets, finds the best tight window within CAL_POINT_MAX_SPREAD_MM, and uses its median.
- If no 2-sample cluster exists, it uses the single good reading closest to the requested calibration point instead of returning 4006.
- 4004 is still used only when there are no usable packets at all.

Purpose: make calibration complete reliably at 2 MHz under real jitter while preserving the v23/v31 measurement path.
