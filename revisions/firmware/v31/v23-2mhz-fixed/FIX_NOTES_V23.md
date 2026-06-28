# NearSens v23 offset mode fix

Changes compared with v22:

1. Fixed the real reason why TS4 could look like it changed nothing. The 75 mm offset was applied inside `applyCalibration()`, so when calibration was invalid or unused, fallback `rawMm` bypassed the offset completely.
2. `applyCalibration()` now returns only the calibrated result from the sensor front. It does not touch FRONT/BODY mode and does not subtract 75 mm.
3. The 75 mm offset is applied exactly once in `showMeasurementResult()`:
   - RAW mode: no calibration and no offset,
   - normal calibrated result: offset only when TS4 sets BODY,
   - normal result without valid calibration: also with offset if BODY is enabled.
4. The offset direction is subtractive: FRONT shows `frontMm`, BODY shows `frontMm - 75 mm`, limited to 0.
5. The TS4 banner was changed to the requested UI: `0 _ 75` = 75 mm offset disabled, `1 _ 75` = 75 mm offset enabled. The second digit is blanked with BCD `0x0F`.

Test:

1. Disable RAW, because RAW intentionally does not show the offset.
2. Make a measurement, for example at 300 mm in FRONT mode.
3. Press TS4. It should show `1 _ 75`.
4. Make the same measurement. The result should be about 75 mm lower.
5. Press TS4. It should show `0 _ 75`.
6. Make the same measurement. The result should return to the value without offset.

Note: if you are in RAW mode, TS4 will not change the reading because RAW by definition shows raw measurement without calibration and without offset.
