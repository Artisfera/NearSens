# NearSens v25 final safe

This version reverts two changes from v24 that made measurement worse and caused calibration problems.

Changes:

- Calibration no longer searches for the cluster closest to the target at any cost.
- The measurement path goes back to the safer v23 style.
- The UI stays the same: `0 _ 75` means no subtraction, `1 _ 75` means 75 mm is subtracted from the displayed result.
- RAW mode still bypasses calibration and offset.
- EEPROM version was changed to force a fresh calibration.

Test:

1. Upload the firmware.
2. Check 8888 after startup.
3. Check RAW at known distances.
4. Calibrate points again.
5. Compare RAW and calibrated results.
