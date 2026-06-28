# NearSens v28 1 MHz calibration stable

Goal of this version: improve calibration at 1 MHz without returning to the v24/v25 errors.

Changes:

- Calibration was adjusted for more stable readings.
- The measurement path remains close to the stable v23/v27 logic.
- RAW mode remains a direct diagnostic readout.
- 75 mm correction is still only a display reference correction.
- EEPROM version was updated to force recalibration if needed.

Test:

Check power-up, RAW readings, calibration point storage and normal mode readings after calibration.
