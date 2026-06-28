# NearSens v29 1 MHz far calibration fix

This version adjusts far-point calibration behaviour while keeping the safer measurement logic.

Changes:

- Far calibration handling was made more tolerant.
- The normal measurement path stays conservative.
- RAW remains useful for checking whether the issue is in detection or calibration.
- 75 mm correction stays outside RAW and calibration.

Test:

Check RAW first, then calibrate far points and compare the calibrated result with the raw reading.
