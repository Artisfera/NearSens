# NearSens v27 1 MHz stable

Changes:

- Cleaned up the 1 MHz configuration.
- Kept the measurement path close to the v23 logic.
- Kept the 75 mm reference UI.
- Kept RAW as a diagnostic mode without calibration and without offset.
- Calibration still stores points in EEPROM.

Test:

1. Upload the firmware with the intended 1 MHz clock setting.
2. Check startup code 8888.
3. Check RAW mode.
4. Calibrate selected points.
5. Check normal mode measurements.
