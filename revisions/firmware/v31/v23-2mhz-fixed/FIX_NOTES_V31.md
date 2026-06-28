# NearSens v31 v23 2 MHz fixed

The base is clean v23 logic because it was the last practically working version.

Changes:

- Adapted the profile for 2 MHz tests.
- Kept the stable v23 measurement path.
- Kept TS4 offset state `0 _ 75` / `1 _ 75`.
- Kept RAW mode and selectable calibration point.
- No forced F_CPU.

Test:

Do not tune the 3-5 cm range. Test 10, 20, 30, 40 and 50 cm.
