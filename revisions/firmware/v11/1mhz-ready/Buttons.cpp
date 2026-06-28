#include "NearSens.h"

void serviceButtons() {
  uint32_t nowMs = millis();
  bool ts3Pressed = (VPORTA.IN & PIN_TS3_bm) == 0;
  bool ts4Pressed = (VPORTA.IN & PIN_TS4_bm) == 0;

  if ((ts3Pressed && !ts3WasPressed) || (ts4Pressed && !ts4WasPressed)) {
    markUserActivity();
  }

  if (!dueMs(nowMs, startupUntilMs) || !dueMs(nowMs, buttonLockoutUntilMs)) {
    ts3WasPressed = ts3Pressed;
    ts4WasPressed = ts4Pressed;
    return;
  }

  if (ts3Pressed && ts4Pressed) {
    if (!chordWasPressed) {
      chordHoldStartMs = nowMs;
      chordHoldTriggered = false;
    }

    chordWasPressed = true;
    if (!chordHoldTriggered && dueMs(nowMs, chordHoldStartMs + CAL_HOLD_MS)) {
      chordHoldTriggered = true;
      ts3HoldTriggered = true;
      ts4HoldTriggered = true;
      buttonLockoutUntilMs = nowMs + BUTTON_LOCKOUT_MS;
      markUserActivity();
      runCalibrationPoint(CAL_500_TARGET_MM);
    }

    ts3WasPressed = true;
    ts4WasPressed = true;
    return;
  }

  if (chordWasPressed) {
    if (!ts3Pressed && !ts4Pressed) {
      chordWasPressed = false;
      chordHoldTriggered = false;
    }
    ts3WasPressed = ts3Pressed;
    ts4WasPressed = ts4Pressed;
    return;
  }

  if (ts3Pressed && !ts3WasPressed) {
    ts3HoldStartMs = nowMs;
    ts3HoldTriggered = false;
  }
  if (ts4Pressed && !ts4WasPressed) {
    ts4HoldStartMs = nowMs;
    ts4HoldTriggered = false;
  }

  // A single long press does not start calibration.
  // Final calibration is only under TS3+TS4 held for 5 s at 500 mm.
  // To blokuje przypadkowe storageanie starego point 100/300 mm.
  if (ts3Pressed && !ts3HoldTriggered &&
      dueMs(nowMs, ts3HoldStartMs + CAL_HOLD_MS)) {
    ts3HoldTriggered = true;
    buttonLockoutUntilMs = nowMs + BUTTON_LOCKOUT_MS;
    ts3WasPressed = ts3Pressed;
    ts4WasPressed = ts4Pressed;
    return;
  }

  if (ts4Pressed && !ts4HoldTriggered &&
      dueMs(nowMs, ts4HoldStartMs + CAL_HOLD_MS)) {
    ts4HoldTriggered = true;
    buttonLockoutUntilMs = nowMs + BUTTON_LOCKOUT_MS;
    ts3WasPressed = ts3Pressed;
    ts4WasPressed = ts4Pressed;
    return;
  }

  if (!ts3Pressed && ts3WasPressed && !ts3HoldTriggered && !ts4Pressed) {
    markUserActivity();
    runUserMeasurement();
    buttonLockoutUntilMs = millis() + BUTTON_LOCKOUT_MS;
  }

  if (!ts4Pressed && ts4WasPressed && !ts4HoldTriggered && !ts3Pressed) {
    markUserActivity();
    toggleReferenceMode();
    buttonLockoutUntilMs = millis() + BUTTON_LOCKOUT_MS;
  }

  ts3WasPressed = ts3Pressed;
  ts4WasPressed = ts4Pressed;
}

void runUserMeasurement() {
  bool ok = false;
  uint16_t rawMm = measurePacket(RUN_MEASURE, &ok);

  if (ok) {
    lastRawMeasurementMm = rawMm;
    lastMeasurementValid = true;
    setDisplayValue(applyCalibration(rawMm));
  } else {
    lastMeasurementValid = false;
    setDisplayValue(rawMm);
  }
}

void toggleReferenceMode() {
  if (referenceMode == REFERENCE_FRONT) {
    referenceMode = REFERENCE_BODY;
  } else {
    referenceMode = REFERENCE_FRONT;
  }

  showReferenceBanner();
}
