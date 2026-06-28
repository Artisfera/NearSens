# NearSens v33 calibrated truth fix

Base: v32 codebase, but NearSens.h profile was replaced with the user-supplied v24-style header and EEPROM_VERSION was bumped to 18.

Main fix:

- Calibration no longer chooses the sample closest to the requested target.
- Calibration stores the best real cluster, or the median of real readings if no cluster exists.
- This prevents fake-perfect 100 mm calibration that can make 80/110/130/140 mm all collapse toward 100 mm.

Measurement fix:

- Packet cluster selection no longer blindly takes the first stable cluster.
- It chooses the compact cluster with most members, then smallest spread, then earlier median as tie-break.

Kept:

- peakUs-based ToF
- TS4 offset state 0 _ 75 / 1 _ 75
- RAW mode
- selectable calibration point
- no forced F_CPU
