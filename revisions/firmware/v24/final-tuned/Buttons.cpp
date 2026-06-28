#include "NearSens.h"

static uint16_t applyDisplayReferenceOffset(uint16_t frontMm) {
  if (referenceMode != REFERENCE_BODY) {
    return frontMm;
  }

  // BODY mode means: show distance corrected by the selected 75 mm mechonical
  // reference. In this project that correction is subtractive. Apply it only to
  // the normal displayed result, never to RAW and never to stored calibration points.
  if (frontMm <= DEVICE_REFERENCE_OFFSET_MM) {
    return 0;
  }

  return (uint16_t)(frontMm - DEVICE_REFERENCE_OFFSET_MM);
}

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

  if (calibrationWizardActive) {
    if (ts3HoldTriggered || ts4HoldTriggered) {
      if (!ts3Pressed && !ts4Pressed) {
        ts3HoldTriggered = false;
        ts4HoldTriggered = false;
        chordWasPressed = false;
        chordHoldTriggered = false;
      }
      ts3WasPressed = ts3Pressed;
      ts4WasPressed = ts4Pressed;
      return;
    }

    if (!ts3Pressed && ts3WasPressed && !ts4Pressed) {
      markUserActivity();
      handleCalibrationWizardConfirm();
      buttonLockoutUntilMs = millis() + BUTTON_LOCKOUT_MS;
    } else if (!ts4Pressed && ts4WasPressed && !ts3Pressed) {
      markUserActivity();
      nextCalibrationWizardPoint();
      buttonLockoutUntilMs = millis() + BUTTON_LOCKOUT_MS;
    }

    ts3WasPressed = ts3Pressed;
    ts4WasPressed = ts4Pressed;
    return;
  }

  if (ts3Pressed && ts4Pressed) {
    ts3ClickCount = 0;
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
      startCalibrationWizard();
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

  serviceTs3ClickAction(nowMs);

  if (ts3Pressed && !ts3WasPressed) {
    ts3HoldStartMs = nowMs;
    ts3HoldTriggered = false;
  }
  if (ts4Pressed && !ts4WasPressed) {
    ts4HoldStartMs = nowMs;
    ts4HoldTriggered = false;
  }

  if (!ts3Pressed && ts3WasPressed && !ts4Pressed) {
    uint32_t pressMs = nowMs - ts3HoldStartMs;
    if (pressMs <= BUTTON_MAX_SHORT_PRESS_MS) {
      markUserActivity();
      registerTs3ShortClick(nowMs);
    }
  }

  if (!ts4Pressed && ts4WasPressed && !ts3Pressed) {
    uint32_t pressMs = nowMs - ts4HoldStartMs;
    if (pressMs <= BUTTON_MAX_SHORT_PRESS_MS) {
      markUserActivity();
      toggleReferenceMode();
      buttonLockoutUntilMs = millis() + BUTTON_LOCKOUT_MS;
    }
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
    showMeasurementResult(rawMm);
  } else {
    lastMeasurementValid = false;
    setDisplayValue(rawMm);
  }
}

void showMeasurementResult(uint16_t rawMm) {
  if (rawMode) {
    // RAW is diagnostic: show the measurement before calibration.
    // The user-selected 75 mm display offset is still applied to the shown value,
    // so LCD state 1 _ 75 always means "subtract 75 mm from what is displayed".
    setDisplayValue(applyDisplayReferenceOffset(rawMm));
    return;
  }

  uint16_t frontMm = rawMm;
  uint16_t calibratedMm = 0;
  if (applyCalibration(rawMm, &calibratedMm)) {
    frontMm = calibratedMm;
  }

  setDisplayValue(applyDisplayReferenceOffset(frontMm));
}

void registerTs3ShortClick(uint32_t nowMs) {
  if (ts3ClickCount == 0 || dueMs(nowMs, ts3ClickFirstMs + TS3_CLICK_WINDOW_MS)) {
    ts3ClickFirstMs = nowMs;
    ts3ClickCount = 0;
  }

  ts3ClickCount++;

  if (ts3ClickCount >= TS3_RAW_CLICK_COUNT) {
    ts3ClickCount = 0;
    toggleRawMode();
    buttonLockoutUntilMs = nowMs + BUTTON_LOCKOUT_MS;
  }
}

void serviceTs3ClickAction(uint32_t nowMs) {
  if (ts3ClickCount == 0) {
    return;
  }

  if (!dueMs(nowMs, ts3ClickFirstMs + TS3_CLICK_WINDOW_MS)) {
    return;
  }

  uint8_t clickCount = ts3ClickCount;
  ts3ClickCount = 0;

  if (clickCount == 1) {
    markUserActivity();
    runUserMeasurement();
    buttonLockoutUntilMs = millis() + BUTTON_LOCKOUT_MS;
  }
}

void toggleReferenceMode() {
  if (referenceMode == REFERENCE_FRONT) {
    referenceMode = REFERENCE_BODY;
  } else {
    referenceMode = REFERENCE_FRONT;
  }

  // UI-only banner. The actual 75 mm offset is applied once in showMeasurementResult().
  showReferenceBanner();
}

void toggleRawMode() {
  rawMode = !rawMode;
  showRawModeBanner();
}
