# NearSens v22 offset fix

This version fixes body reference mode logic after practical testing.

## Changes

1. `DEVICE_REFERENCE_OFFSET_MM = 75` is no longer added.
2. In `REFERENCE_BODY` mode, the offset is subtracted from the calibrated result.
3. The offset is applied exactly once and only to the normal calibrated result.
4. RAW mode has no offset.
5. Calibration samples have no offset.
6. `REFERENCE_FRONT` mode applies no offset.
7. The reference mode banner is explicit: `2001` = front / no offset, `2002` = body / subtract 75 mm.

## Fix location

The main change is in `Calibration.cpp`, in `applyCalibration()`.

Previously it was:

```cpp
if (referenceMode == REFERENCE_BODY) {
  corrected += DEVICE_REFERENCE_OFFSET_MM;
}
```

Now it is:

```cpp
if (referenceMode == REFERENCE_BODY) {
  corrected -= DEVICE_REFERENCE_OFFSET_MM;
}
```

The code comment also explains that the offset does not apply to RAW or calibration.

## Test

1. Upload the firmware.
2. After startup it should show `8888`.
3. TS4 should toggle: `2001` front, `2002` body.
4. In front mode, the result should have no offset.
5. In body mode, the result should be 75 mm lower.
6. RAW mode should show the same raw result regardless of reference mode.
