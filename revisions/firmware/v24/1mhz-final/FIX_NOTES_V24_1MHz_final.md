# NearSens v24 1 MHz final

This package is a full Arduino sketch, not a single header file.

Settings before upload:

1. Board: ATtiny1626 with megaTinyCore.
2. Clock: 1 MHz internal.
3. Programmer/upload: same as before, jtag2updi/UPDI.
4. After upload, old calibration will be ignored because EEPROM_VERSION was bumped to 11.

What changed compared with v23:

1. The whole package was fixed, not only NearSens.h.
2. ADC is configured directly for fast reads at 1 MHz.
3. 100/300/500 mm calibration selects the cluster closest to the known calibration point, instead of always taking the first stable cluster.
4. A check was added so calibration does not store a stable false 300 mm echo as the 500 mm point.
5. Measurement and calibration clusters are separated. Normal measurement is tighter, calibration is more tolerant.
6. The 75 mm offset still works only in the normal result. RAW has no offset and no calibration.

Test procedure:

1. Upload the firmware.
2. After startup, check `8888`.
3. Enable RAW and check distances.
4. Enter calibration.
5. Store the selected calibration points.
6. Compare RAW and normal mode results.
7. Store 300 mm as the first point.
