# NearSens v30 from v23 1 MHz real

This version returns to the practical v23-style measurement path and adapts it for real 1 MHz tests.

Changes:

- Kept peakUs-based time-of-flight logic.
- Avoided heavy detection changes from later experimental versions.
- Kept TS3, TS4, RAW mode and the 75 mm reference UI.
- Kept a conservative receive window for the 10-50 cm range.

Test:

Do not tune for the 3-5 cm range. Test 10, 20, 30, 40 and 50 cm.
