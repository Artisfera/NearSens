# NearSens v24 five-point calibration

Changes compared with v23:

1. Calibration now has five points: 100, 200, 300, 400 and 500 mm.
2. The calibration wizard supports the cyclic point list `100 -> 200 -> 300 -> 400 -> 500`. The start is still set to 300 mm because it is the most practical correction point. After entering the wizard, TS4 goes through `400 -> 500 -> 100 -> 200 -> 300`.
3. EEPROM format was bumped to v11 and stores all five raw calibration values and five validity flags.
4. Old EEPROM v10 data with 100/300/500 mm points is read and migrated to v11 on first startup after update.
5. Measurement correction selects the best available pair of calibration points. If only one valid point is available, the firmware still uses single-point offset correction as in the previous fallback behavior.

Test after upload:

1. Enter calibration by holding TS3+TS4.
2. Select point 100, 200, 300, 400 or 500 mm with TS4.
3. Confirm the measurement for the selected point with TS3.
4. After all points are calibrated, check control measurements at 100, 200, 300, 400 and 500 mm.
5. RAW mode still shows raw measurement without calibration and without the 75 mm offset.
