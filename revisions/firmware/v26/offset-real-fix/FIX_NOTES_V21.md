# NearSens v21 final candidate

Goal of this version: simplify and stabilize RAW/measurement without removing user-facing functions.

Main changes compared with v20:

1. `measureOnePing()` no longer ends measurement at the first threshold crossing. It now scans the full echo window, collects pulse candidates and selects the strongest qualified peak.
2. During the critical ADC scan, BP/PH LCD is not serviced. BP/PH works normally outside the short receive window, but it does not switch LCD pins while sampling a weak RX signal.
3. Clusters were narrowed: `MEASURE_CLUSTER_MM = 10`, `CAL_CLUSTER_MM = 10`. This makes the firmware hide sample spread less often as a "stable" result.
4. A stricter minimum sample count was set: `MEASURE_MIN_VALID = 5`, `MEASURE_MIN_CLUSTER = 4`, `CAL_MIN_VALID = 5`, `CAL_MIN_CLUSTER = 4`.
5. The echo window was increased to `ECHO_TIMEOUT_FROM_TX_START_US = 6200UL`, but without returning to the very wide window from old versions. The goal is margin for 50 cm and analog path delay without catching far secondary reflections.
6. EEPROM version is `10`, so calibration must be repeated after uploading.

Kept features:

- TS3 as measurement.
- TS4 as front/body reference toggle.
- `DEVICE_REFERENCE_OFFSET_MM = 75`.
- RAW mode through five TS3 clicks.
- Idle `8888`.
- Calibration with point selection using TS4.
- 40xx codes.
- BP/PH LCD.
- No forced `F_CPU`. The clock is selected in Arduino IDE / megaTinyCore.

Test procedure:

1. Upload the firmware.
2. After startup, check `8888`.
3. Enable RAW with five TS3 clicks.
4. Measure 20, 25, 30, 35, 40, 45 and 50 cm in RAW.
5. Enter calibration by holding TS3+TS4 for about 5 s.
6. Select the calibration point with TS4.
7. Start by calibrating `0300`.
8. Then add `0100` and `0500` if you want full 3-point interpolation.
9. Measure 20, 25, 30, 35, 40, 45 and 50 cm again in RAW and normal mode.

If RAW still has a large error, the problem is in echo detection or the RX path, not in calibration. If RAW is good but normal mode is bad, the problem is in calibration or EEPROM data.
