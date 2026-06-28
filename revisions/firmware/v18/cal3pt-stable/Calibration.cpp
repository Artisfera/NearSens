#include <EEPROM.h>

#include "NearSens.h"

bool applyCalibration(uint16_t rawMm, uint16_t *correctedMm) {
  if (correctedMm == 0) {
    return false;
  }

  if (!calibrationValid ||
      !calibrationSetValid(calibrationRaw100Mm,
                           calibrationRaw300Mm,
                           calibrationRaw500Mm)) {
    return false;
  }

  uint16_t calibratedMm = 0;
  bool ok = false;

  if (rawMm <= calibrationRaw300Mm) {
    ok = interpolateCalibratedValue(rawMm,
                                    calibrationRaw100Mm, CAL_100_TARGET_MM,
                                    calibrationRaw300Mm, CAL_300_TARGET_MM,
                                    &calibratedMm);
  } else {
    ok = interpolateCalibratedValue(rawMm,
                                    calibrationRaw300Mm, CAL_300_TARGET_MM,
                                    calibrationRaw500Mm, CAL_500_TARGET_MM,
                                    &calibratedMm);
  }

  if (!ok) {
    return false;
  }

  int32_t corrected = calibratedMm;
  if (referenceMode == REFERENCE_BODY) {
    /*
      Positive offset reports distance from the rear/body reference point:
      object distance from sensor face plus the measured 75 mm body length.
    */
    corrected += (int32_t)DEVICE_REFERENCE_OFFSET_MM;
  }

  if (corrected < 0) {
    *correctedMm = 0;
  } else if (corrected > LCD_DISPLAY_MAX) {
    *correctedMm = LCD_DISPLAY_MAX;
  } else {
    *correctedMm = (uint16_t)corrected;
  }

  return true;
}

bool interpolateCalibratedValue(int32_t rawMm, int32_t rawA, int32_t targetA, int32_t rawB, int32_t targetB, uint16_t *correctedMm) {
  if (correctedMm == 0) {
    return false;
  }

  int32_t rawSpan = rawB - rawA;
  if (rawSpan <= 0) {
    return false;
  }

  int32_t targetSpan = targetB - targetA;
  int64_t numerator = (int64_t)(rawMm - rawA) * (int64_t)targetSpan;

  if (numerator >= 0) {
    numerator += rawSpan / 2;
  } else {
    numerator -= rawSpan / 2;
  }

  int32_t corrected = targetA + (int32_t)(numerator / rawSpan);
  if (corrected < 0) {
    corrected = 0;
  }
  if (corrected > LCD_DISPLAY_MAX) {
    corrected = LCD_DISPLAY_MAX;
  }

  *correctedMm = (uint16_t)corrected;
  return true;
}

bool calibrationSetValid(uint16_t raw100Mm, uint16_t raw300Mm, uint16_t raw500Mm) {
  if (!(raw100Mm < raw300Mm && raw300Mm < raw500Mm)) {
    return false;
  }

  if ((uint16_t)(raw300Mm - raw100Mm) < CAL_RAW_MIN_SEPARATION_100_300_MM) {
    return false;
  }
  if ((uint16_t)(raw500Mm - raw300Mm) < CAL_RAW_MIN_SEPARATION_300_500_MM) {
    return false;
  }

  if (!calibrationRawInWindow(raw100Mm, CAL_VALID_RAW100_MIN_MM, CAL_VALID_RAW100_MAX_MM)) {
    return false;
  }
  if (!calibrationRawInWindow(raw300Mm, CAL_VALID_RAW300_MIN_MM, CAL_VALID_RAW300_MAX_MM)) {
    return false;
  }
  if (!calibrationRawInWindow(raw500Mm, CAL_VALID_RAW500_MIN_MM, CAL_VALID_RAW500_MAX_MM)) {
    return false;
  }

  return true;
}

bool calibrationRawInWindow(uint16_t rawMm, uint16_t minMm, uint16_t maxMm) {
  return rawMm >= minMm && rawMm <= maxMm;
}

void startCalibrationWizard() {
  calibrationWizardActive = true;
  calibrationWizardStep = 0;
  ts3ClickCount = 0;
  lastMeasurementValid = false;
  pendingCalibrationRaw100Mm = CAL_100_TARGET_MM;
  pendingCalibrationRaw300Mm = CAL_300_TARGET_MM;
  pendingCalibrationRaw500Mm = CAL_500_TARGET_MM;
  showCalibrationWizardPrompt();
}

void cancelCalibrationWizard() {
  calibrationWizardActive = false;
  calibrationWizardStep = 0;
  ts3ClickCount = 0;
  showIdleMarker();
}

void showCalibrationWizardPrompt() {
  if (calibrationWizardStep == 0) {
    setDisplayValue(CAL_100_TARGET_MM);
  } else if (calibrationWizardStep == 1) {
    setDisplayValue(CAL_300_TARGET_MM);
  } else {
    setDisplayValue(CAL_500_TARGET_MM);
  }
}

void handleCalibrationWizardConfirm() {
  uint16_t targetMm = CAL_100_TARGET_MM;
  if (calibrationWizardStep == 1) {
    targetMm = CAL_300_TARGET_MM;
  } else if (calibrationWizardStep >= 2) {
    targetMm = CAL_500_TARGET_MM;
  }

  uint16_t rawMm = 0;
  uint16_t errorCode = LCD_CAL_FAIL;
  if (!captureCalibrationPoint(targetMm, &rawMm, &errorCode)) {
    calibrationWizardActive = false;
    setDisplayValue(errorCode);
    return;
  }

  if (calibrationWizardStep == 0) {
    pendingCalibrationRaw100Mm = rawMm;
    calibrationWizardStep = 1;
    showCalibrationWizardPrompt();
    return;
  }

  if (calibrationWizardStep == 1) {
    pendingCalibrationRaw300Mm = rawMm;
    calibrationWizardStep = 2;
    showCalibrationWizardPrompt();
    return;
  }

  pendingCalibrationRaw500Mm = rawMm;
  calibrationWizardActive = false;

  if (!calibrationSetValid(pendingCalibrationRaw100Mm,
                           pendingCalibrationRaw300Mm,
                           pendingCalibrationRaw500Mm)) {
    setDisplayValue(LCD_CAL_FAIL);
    return;
  }

  calibrationRaw100Mm = pendingCalibrationRaw100Mm;
  calibrationRaw300Mm = pendingCalibrationRaw300Mm;
  calibrationRaw500Mm = pendingCalibrationRaw500Mm;
  calibrationValid = true;
  saveCalibration();
  lastMeasurementValid = false;
  setDisplayValue(LCD_CAL_SUCCESS);
}

bool captureCalibrationPoint(uint16_t targetMm, uint16_t *rawMm, uint16_t *errorCode) {
  uint16_t samples[CAL_POINT_SAMPLES];
  uint8_t goodCount = 0;

  for (uint8_t i = 0; i < CAL_POINT_SAMPLES; i++) {
    setDisplayValue(targetMm);
    bool ok = false;
    uint16_t sample = measurePacket(RUN_CALIBRATE, &ok);
    if (ok && goodCount < CAL_POINT_SAMPLES) {
      samples[goodCount++] = sample;
    }

    uint32_t waitUntilMs = millis() + 120UL;
    while (!dueMs(millis(), waitUntilMs)) {
      serviceBackplane();
    }
  }

  if (goodCount < CAL_POINT_MIN_GOOD) {
    *errorCode = LCD_CAL_FAIL;
    return false;
  }

  sortValues(samples, goodCount);
  uint16_t spread = samples[goodCount - 1] - samples[0];
  if (spread > CAL_POINT_MAX_SPREAD_MM) {
    *errorCode = LCD_CAL_SPREAD;
    return false;
  }

  *rawMm = samples[goodCount / 2];
  return true;
}

void loadCalibration() {
  referenceMode = REFERENCE_FRONT;
  rawMode = false;
  calibrationRaw100Mm = CAL_100_TARGET_MM;
  calibrationRaw300Mm = CAL_300_TARGET_MM;
  calibrationRaw500Mm = CAL_500_TARGET_MM;
  calibrationValid = false;

  (void)loadCalibrationV8();
}

bool loadCalibrationV8() {
  uint8_t magic0 = EEPROM.read(EEPROM_MAGIC0_ADDR);
  uint8_t magic1 = EEPROM.read(EEPROM_MAGIC1_ADDR);
  uint8_t version = EEPROM.read(EEPROM_VERSION_ADDR);
  uint16_t storedRaw100 = (uint16_t)EEPROM.read(EEPROM_RAW100_L_ADDR) |
                          ((uint16_t)EEPROM.read(EEPROM_RAW100_H_ADDR) << 8);
  uint16_t storedRaw300 = (uint16_t)EEPROM.read(EEPROM_RAW300_L_ADDR) |
                          ((uint16_t)EEPROM.read(EEPROM_RAW300_H_ADDR) << 8);
  uint16_t storedRaw500 = (uint16_t)EEPROM.read(EEPROM_RAW500_L_ADDR) |
                          ((uint16_t)EEPROM.read(EEPROM_RAW500_H_ADDR) << 8);
  uint8_t flags = EEPROM.read(EEPROM_FLAGS_ADDR);
  uint8_t storedChecksum = EEPROM.read(EEPROM_CHECKSUM_ADDR);

  if (magic0 != EEPROM_MAGIC0 || magic1 != EEPROM_MAGIC1 ||
      version != EEPROM_VERSION ||
      flags != EEPROM_FLAGS_ALL_VALID ||
      storedChecksum != calibrationChecksumV8(storedRaw100, storedRaw300, storedRaw500, flags) ||
      !calibrationSetValid(storedRaw100, storedRaw300, storedRaw500)) {
    calibrationValid = false;
    return false;
  }

  calibrationRaw100Mm = storedRaw100;
  calibrationRaw300Mm = storedRaw300;
  calibrationRaw500Mm = storedRaw500;
  calibrationValid = true;
  return true;
}

void saveCalibration() {
  uint8_t flags = EEPROM_FLAGS_ALL_VALID;

  EEPROM.update(EEPROM_MAGIC0_ADDR, EEPROM_MAGIC0);
  EEPROM.update(EEPROM_MAGIC1_ADDR, EEPROM_MAGIC1);
  EEPROM.update(EEPROM_VERSION_ADDR, EEPROM_VERSION);
  EEPROM.update(EEPROM_RAW100_L_ADDR, (uint8_t)(calibrationRaw100Mm & 0xFF));
  EEPROM.update(EEPROM_RAW100_H_ADDR, (uint8_t)((calibrationRaw100Mm >> 8) & 0xFF));
  EEPROM.update(EEPROM_RAW300_L_ADDR, (uint8_t)(calibrationRaw300Mm & 0xFF));
  EEPROM.update(EEPROM_RAW300_H_ADDR, (uint8_t)((calibrationRaw300Mm >> 8) & 0xFF));
  EEPROM.update(EEPROM_RAW500_L_ADDR, (uint8_t)(calibrationRaw500Mm & 0xFF));
  EEPROM.update(EEPROM_RAW500_H_ADDR, (uint8_t)((calibrationRaw500Mm >> 8) & 0xFF));
  EEPROM.update(EEPROM_FLAGS_ADDR, flags);
  EEPROM.update(EEPROM_CHECKSUM_ADDR, calibrationChecksumV8(calibrationRaw100Mm,
                                                            calibrationRaw300Mm,
                                                            calibrationRaw500Mm,
                                                            flags));
}

uint8_t calibrationChecksumV8(uint16_t raw100Mm, uint16_t raw300Mm, uint16_t raw500Mm, uint8_t flags) {
  return EEPROM_MAGIC0 ^ EEPROM_MAGIC1 ^ EEPROM_VERSION ^
         (uint8_t)(raw100Mm & 0xFF) ^
         (uint8_t)((raw100Mm >> 8) & 0xFF) ^
         (uint8_t)(raw300Mm & 0xFF) ^
         (uint8_t)((raw300Mm >> 8) & 0xFF) ^
         (uint8_t)(raw500Mm & 0xFF) ^
         (uint8_t)((raw500Mm >> 8) & 0xFF) ^
         flags ^ 0x5A;
}
