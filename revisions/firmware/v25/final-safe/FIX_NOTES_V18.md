# NearSens v18 stable 3-point calibration

Changes compared with v17_cal2pt:

- restored 3-point calibration at 100/300/500 mm,
- calibration works as direct piecewise interpolation from raw value to target value,
- calibration correction is not clamped,
- EEPROM was bumped to version 8 and stores raw100/raw300/raw500,
- TS3+TS4 now runs the 0100 -> 0300 -> 0500 wizard,
- measurement and calibration clusters were narrowed,
- far fallback was disabled,
- echo timeout was shortened to a range useful for 10-50 cm plus margin,
- frontend functions were kept: TS3, TS4, RAW, 75 mm offset, idle 8888, 40xx codes,
- the compile warning for F_CPU other than 1 MHz was removed. Select the clock in Arduino IDE/megaTinyCore.

After uploading, perform a full new calibration because EEPROM v7 from v17 is not compatible with v18.

Minimum test:

1. Upload the firmware.
2. After startup it should show 8888.
3. Enter calibration with TS3+TS4 held for about 5 s.
4. Calibrate 0100, 0300 and 0500.
5. Check 100, 300 and 500 mm.
6. Check 350 and 400 mm.
7. Enable RAW with five TS3 clicks.

If RAW at 350 mm shows about 300 mm, the problem is in echo detection or the RX path. If RAW shows about 350 mm but the calibrated result does not, the problem is in calibration.
