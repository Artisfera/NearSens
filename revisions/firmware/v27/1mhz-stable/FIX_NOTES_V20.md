# NearSens v20 simple practical fix

Main changes:

1. Calibration no longer requires all three points to be done at once. After entering calibration, TS4 selects point 0100/0300/0500 and TS3 stores only the currently selected point.
2. EEPROM v9 stores raw100, raw300, raw500 and validity flags for individual points. One point gives offset correction, two points give a line, and three points give piecewise 100/300/500 interpolation.
3. Measurement selects the first stable distance cluster, not the largest one. This limits the case where a false repeatable peak around 300 mm wins against the real closer target.
4. Frontend functions remain: TS3, TS4, +75 mm body reference, RAW, idle 8888 and 40xx codes.
5. The code does not force F_CPU. Select the clock in megaTinyCore/Arduino IDE settings.

Practical test:

- Upload the firmware.
- Check 8888.
- Enter RAW with five TS3 clicks.
- Check RAW at 25 cm and 30 cm. If RAW at 25 cm still shows 30 cm, the problem is in echo detection or the RX path, not in calibration.
- Enter calibration with TS3+TS4 held for 5 s.
- Select 0300 with TS4, place the object at 30 cm, then store it with TS3.
- Measure 25 cm.
- If the middle point works, add 0100 and 0500 separately.
