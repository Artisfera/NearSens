# NearSens v26 offset real fix

This version is based on v23 because v24/v25 introduced echo detection changes that made the 30 cm result worse after calibration.

Main idea:

- Keep the v23 measurement path.
- Keep the 75 mm UI: `0 _ 75` = no subtraction, `1 _ 75` = subtract 75 mm.
- Apply the offset only to the displayed normal result.
- Do not apply offset to RAW or calibration samples.
- Keep EEPROM changes only where needed.

Test:

Upload the firmware, check RAW first, then calibrate and compare normal mode with RAW.
