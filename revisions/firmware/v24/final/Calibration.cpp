#include <EEPROM.h>

#include "NearSens.h"

static const uint16_t CAL_TARGETS[CAL_POINT_COUNT] = {
  CAL_100_TARGET_MM,
  CAL_200_TARGET_MM,
  CAL_300_TARGET_MM,
  CAL_400_TARGET_MM,
  CAL_500_TARGET_MM
};

static const uint8_t CAL_FLAGS[CAL_POINT_COUNT] = {
  EEPROM_FLAG_100_VALID,
  EEPROM_FLAG_200_VALID,
  EEPROM_FLAG_300_VALID,
  EEPROM_FLAG_400_VALID,
  EEPROM_FLAG_500_VALID
};

static uint16_t calibrationTargetForStep(uint8_t step) {
  return CAL_TARGETS[step % CAL_POINT_COUNT];
}

static uint8_t calibrationIndexForTarget(uint16_t targetMm) {
  for (uint8_t i = 0; i < CAL_POINT_COUNT; i++) {
    if (CAL_TARGETS[i] == targetMm) {
      return i;
    }
  }
  return 0;
}

static uint8_t calibrationIndexForFlag(uint8_t flag) {
  for (uint8_t i = 0; i < CAL_POINT_COUNT; i++) {
    if (CAL_FLAGS[i] == flag) {
      return i;
    }
  }
  return 0;
}

static uint8_t calibrationFlagForTarget(uint16_t targetMm) {
  return CAL_FLAGS[calibrationIndexForTarget(targetMm)];
}

static uint16_t *rawStorageForIndex(uint8_t index) {
  if (index == 0) {
    return &calibrationRaw100Mm;
  }
  if (index == 1) {
    return &calibrationRaw200Mm;
  }
  if (index == 2) {
    return &calibrationRaw300Mm;
  }
  if (index == 3) {
    return &calibrationRaw400Mm;
  }
  return &calibrationRaw500Mm;
}

static uint16_t rawForIndex(uint8_t index) {
  return *rawStorageForIndex(index);
}

static uint16_t minSeparationForIndexes(uint8_t indexA, uint8_t indexB) {
  uint8_t steps = (indexB > indexA) ? (indexB - indexA) : (indexA - indexB);
  if (steps == 0) {
    steps = 1;
  }
  return (uint16_t)(CAL_RAW_MIN_SEPARATION_STEP_MM * steps);
}

static bool calibrationPairValidForIndexes(uint8_t indexA, uint8_t indexB) {
  return calibrationPairValid(rawForIndex(indexA),
                              rawForIndex(indexB),
                              minSeparationForIndexes(indexA, indexB));
}

static bool calibrationPointValidForIndex(uint8_t index) {
  return calibrationPointValid(CAL_FLAGS[index], rawForIndex(index));
}

static bool calibrationIndexUsable(uint8_t index) {
  uint8_t flag = CAL_FLAGS[index];
  return (calibrationFlags & flag) != 0 && calibrationPointValidForIndex(index);
}

static void clearConflictingCalibrationNeighbours(uint8_t newIndex) {
  for (uint8_t i = 0; i < CAL_POINT_COUNT; i++) {
    if (i == newIndex || !calibrationIndexUsable(i)) {
      continue;
    }

    uint8_t indexA = (i < newIndex) ? i : newIndex;
    uint8_t indexB = (i < newIndex) ? newIndex : i;
    if (!calibrationPairValidForIndexes(indexA, indexB)) {
      calibrationFlags &= (uint8_t)~CAL_FLAGS[i];
    }
  }
}

static bool interpolateBestCalibrationPair(uint16_t rawMm, uint16_t *correctedMm) {
  bool found = false;
  uint8_t bestA = 0;
  uint8_t bestB = 0;
  uint32_t bestDistance = 0xFFFFFFFFUL;
  uint8_t bestTargetSpan = 255;

  for (uint8_t a = 0; a < CAL_POINT_COUNT; a++) {
    if (!calibrationIndexUsable(a)) {
      continue;
    }

    for (uint8_t b = (uint8_t)(a + 1); b < CAL_POINT_COUNT; b++) {
      if (!calibrationIndexUsable(b)) {
        continue;
      }
      if (!calibrationPairValidForIndexes(a, b)) {
        continue;
      }

      uint16_t rawA = rawForIndex(a);
      uint16_t rawB = rawForIndex(b);
      uint32_t distance = 0;
      if (rawMm < rawA) {
        distance = rawA - rawMm;
      } else if (rawMm > rawB) {
        distance = rawMm - rawB;
      }

      uint8_t targetSpan = b - a;
      if (!found || distance < bestDistance ||
          (distance == bestDistance && targetSpan < bestTargetSpan)) {
        found = true;
        bestA = a;
        bestB = b;
        bestDistance = distance;
        bestTargetSpan = targetSpan;
      }
    }
  }

  if (!found) {
    return false;
  }

  return interpolateCalibratedValue(rawMm,
                                    rawForIndex(bestA), CAL_TARGETS[bestA],
                                    rawForIndex(bestB), CAL_TARGETS[bestB],
                                    correctedMm);
}

static bool applySinglePointOffset(uint16_t rawMm, uint16_t *correctedMm) {
  bool found = false;
  uint8_t bestIndex = 0;
  uint16_t bestDistance = 0xFFFF;

  for (uint8_t i = 0; i < CAL_POINT_COUNT; i++) {
    if (!calibrationIndexUsable(i)) {
      continue;
    }

    uint16_t distance = absDiff16(rawMm, rawForIndex(i));
    if (!found || distance < bestDistance) {
      found = true;
      bestIndex = i;
      bestDistance = distance;
    }
  }

  if (!found) {
    return false;
  }

  int32_t corrected = (int32_t)rawMm +
                      ((int32_t)CAL_TARGETS[bestIndex] - (int32_t)rawForIndex(bestIndex));
  if (corrected < 0) {
    corrected = 0;
  }
  if (corrected > LCD_DISPLAY_MAX) {
    corrected = LCD_DISPLAY_MAX;
  }

  *correctedMm = (uint16_t)corrected;
  return true;
}

bool applyCalibration(uint16_t rawMm, uint16_t *correctedMm) {
  if (correctedMm == 0) {
    return false;
  }

  uint16_t calibratedMm = rawMm;
  bool ok = interpolateBestCalibrationPair(rawMm, &calibratedMm);
  if (!ok) {
    ok = applySinglePointOffset(rawMm, &calibratedMm);
  }

  if (!ok) {
    return false;
  }

  // applyCalibration() returns the calibrated FRONT/reference-from-sensor result only.
  // The 75 mm body/reference offset is intentionally NOT applied here. It is a
  // display-mode correction and is applied exactly once in showMeasurementResult().
  *correctedMm = calibratedMm;
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

bool calibrationFullSetValid(uint16_t raw100Mm, uint16_t raw200Mm, uint16_t raw300Mm, uint16_t raw400Mm, uint16_t raw500Mm) {
  if (!calibrationPointValid(EEPROM_FLAG_100_VALID, raw100Mm) ||
      !calibrationPointValid(EEPROM_FLAG_200_VALID, raw200Mm) ||
      !calibrationPointValid(EEPROM_FLAG_300_VALID, raw300Mm) ||
      !calibrationPointValid(EEPROM_FLAG_400_VALID, raw400Mm) ||
      !calibrationPointValid(EEPROM_FLAG_500_VALID, raw500Mm)) {
    return false;
  }
  if (!calibrationPairValid(raw100Mm, raw200Mm, CAL_RAW_MIN_SEPARATION_STEP_MM)) {
    return false;
  }
  if (!calibrationPairValid(raw200Mm, raw300Mm, CAL_RAW_MIN_SEPARATION_STEP_MM)) {
    return false;
  }
  if (!calibrationPairValid(raw300Mm, raw400Mm, CAL_RAW_MIN_SEPARATION_STEP_MM)) {
    return false;
  }
  if (!calibrationPairValid(raw400Mm, raw500Mm, CAL_RAW_MIN_SEPARATION_STEP_MM)) {
    return false;
  }
  return true;
}

bool calibrationPairValid(uint16_t rawA, uint16_t rawB, uint16_t minSep) {
  if (rawA >= rawB) {
    return false;
  }
  return (uint16_t)(rawB - rawA) >= minSep;
}

bool calibrationPointValid(uint8_t flag, uint16_t rawMm) {
  if (flag == EEPROM_FLAG_100_VALID) {
    return calibrationRawInWindow(rawMm, CAL_VALID_RAW100_MIN_MM, CAL_VALID_RAW100_MAX_MM);
  }
  if (flag == EEPROM_FLAG_200_VALID) {
    return calibrationRawInWindow(rawMm, CAL_VALID_RAW200_MIN_MM, CAL_VALID_RAW200_MAX_MM);
  }
  if (flag == EEPROM_FLAG_300_VALID) {
    return calibrationRawInWindow(rawMm, CAL_VALID_RAW300_MIN_MM, CAL_VALID_RAW300_MAX_MM);
  }
  if (flag == EEPROM_FLAG_400_VALID) {
    return calibrationRawInWindow(rawMm, CAL_VALID_RAW400_MIN_MM, CAL_VALID_RAW400_MAX_MM);
  }
  if (flag == EEPROM_FLAG_500_VALID) {
    return calibrationRawInWindow(rawMm, CAL_VALID_RAW500_MIN_MM, CAL_VALID_RAW500_MAX_MM);
  }
  return false;
}

bool calibrationRawInWindow(uint16_t rawMm, uint16_t minMm, uint16_t maxMm) {
  return rawMm >= minMm && rawMm <= maxMm;
}

void startCalibrationWizard() {
  calibrationWizardActive = true;
  calibrationWizardStep = 2;  // Default to the most useful correction point, 300 mm.
  ts3ClickCount = 0;
  lastMeasurementValid = false;
  showCalibrationWizardPrompt();
}

void cancelCalibrationWizard() {
  calibrationWizardActive = false;
  calibrationWizardStep = 0;
  ts3ClickCount = 0;
  showIdleMarker();
}

void nextCalibrationWizardPoint() {
  calibrationWizardStep++;
  if (calibrationWizardStep >= CAL_POINT_COUNT) {
    calibrationWizardStep = 0;
  }
  showCalibrationWizardPrompt();
}

void showCalibrationWizardPrompt() {
  setDisplayValue(calibrationTargetForStep(calibrationWizardStep));
}

void handleCalibrationWizardConfirm() {
  uint16_t targetMm = calibrationTargetForStep(calibrationWizardStep);
  uint8_t pointFlag = calibrationFlagForTarget(targetMm);
  uint8_t pointIndex = calibrationIndexForFlag(pointFlag);

  uint16_t rawMm = 0;
  uint16_t errorCode = LCD_CAL_FAIL;
  if (!captureCalibrationPoint(targetMm, &rawMm, &errorCode)) {
    calibrationWizardActive = false;
    setDisplayValue(errorCode);
    return;
  }

  if (!calibrationPointValid(pointFlag, rawMm)) {
    calibrationWizardActive = false;
    setDisplayValue(LCD_CAL_FAIL);
    return;
  }

  *rawStorageForIndex(pointIndex) = rawMm;
  calibrationFlags |= pointFlag;
  clearConflictingCalibrationNeighbours(pointIndex);

  calibrationValid = calibrationFlags != 0;
  saveCalibration();
  lastMeasurementValid = false;
  calibrationWizardActive = false;
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

    uint32_t waitUntilMs = millis() + 100UL;
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
  calibrationRaw200Mm = CAL_200_TARGET_MM;
  calibrationRaw300Mm = CAL_300_TARGET_MM;
  calibrationRaw400Mm = CAL_400_TARGET_MM;
  calibrationRaw500Mm = CAL_500_TARGET_MM;
  calibrationFlags = 0;
  calibrationValid = false;

  if (!loadCalibrationV11()) {
    (void)loadCalibrationV10();
  }
}

bool loadCalibrationV11() {
  uint8_t magic0 = EEPROM.read(EEPROM_MAGIC0_ADDR);
  uint8_t magic1 = EEPROM.read(EEPROM_MAGIC1_ADDR);
  uint8_t version = EEPROM.read(EEPROM_VERSION_ADDR);
  uint16_t storedRaw100 = (uint16_t)EEPROM.read(EEPROM_RAW100_L_ADDR) |
                          ((uint16_t)EEPROM.read(EEPROM_RAW100_H_ADDR) << 8);
  uint16_t storedRaw200 = (uint16_t)EEPROM.read(EEPROM_RAW200_L_ADDR) |
                          ((uint16_t)EEPROM.read(EEPROM_RAW200_H_ADDR) << 8);
  uint16_t storedRaw300 = (uint16_t)EEPROM.read(EEPROM_RAW300_L_ADDR) |
                          ((uint16_t)EEPROM.read(EEPROM_RAW300_H_ADDR) << 8);
  uint16_t storedRaw400 = (uint16_t)EEPROM.read(EEPROM_RAW400_L_ADDR) |
                          ((uint16_t)EEPROM.read(EEPROM_RAW400_H_ADDR) << 8);
  uint16_t storedRaw500 = (uint16_t)EEPROM.read(EEPROM_RAW500_L_ADDR) |
                          ((uint16_t)EEPROM.read(EEPROM_RAW500_H_ADDR) << 8);
  uint8_t flags = EEPROM.read(EEPROM_FLAGS_ADDR) & EEPROM_FLAGS_ALL_VALID;
  uint8_t storedChecksum = EEPROM.read(EEPROM_CHECKSUM_ADDR);

  if (magic0 != EEPROM_MAGIC0 || magic1 != EEPROM_MAGIC1 ||
      version != EEPROM_VERSION ||
      storedChecksum != calibrationChecksumV11(storedRaw100, storedRaw200,
                                               storedRaw300, storedRaw400,
                                               storedRaw500, flags)) {
    calibrationValid = false;
    calibrationFlags = 0;
    return false;
  }

  if ((flags & EEPROM_FLAG_100_VALID) != 0 &&
      calibrationPointValid(EEPROM_FLAG_100_VALID, storedRaw100)) {
    calibrationRaw100Mm = storedRaw100;
    calibrationFlags |= EEPROM_FLAG_100_VALID;
  }
  if ((flags & EEPROM_FLAG_200_VALID) != 0 &&
      calibrationPointValid(EEPROM_FLAG_200_VALID, storedRaw200)) {
    calibrationRaw200Mm = storedRaw200;
    calibrationFlags |= EEPROM_FLAG_200_VALID;
  }
  if ((flags & EEPROM_FLAG_300_VALID) != 0 &&
      calibrationPointValid(EEPROM_FLAG_300_VALID, storedRaw300)) {
    calibrationRaw300Mm = storedRaw300;
    calibrationFlags |= EEPROM_FLAG_300_VALID;
  }
  if ((flags & EEPROM_FLAG_400_VALID) != 0 &&
      calibrationPointValid(EEPROM_FLAG_400_VALID, storedRaw400)) {
    calibrationRaw400Mm = storedRaw400;
    calibrationFlags |= EEPROM_FLAG_400_VALID;
  }
  if ((flags & EEPROM_FLAG_500_VALID) != 0 &&
      calibrationPointValid(EEPROM_FLAG_500_VALID, storedRaw500)) {
    calibrationRaw500Mm = storedRaw500;
    calibrationFlags |= EEPROM_FLAG_500_VALID;
  }

  calibrationValid = calibrationFlags != 0;
  return calibrationValid;
}

bool loadCalibrationV10() {
  static const uint8_t EEPROM_V10_VERSION = 10;
  static const uint8_t EEPROM_V10_RAW300_L_ADDR = 5;
  static const uint8_t EEPROM_V10_RAW300_H_ADDR = 6;
  static const uint8_t EEPROM_V10_RAW500_L_ADDR = 7;
  static const uint8_t EEPROM_V10_RAW500_H_ADDR = 8;
  static const uint8_t EEPROM_V10_FLAGS_ADDR = 9;
  static const uint8_t EEPROM_V10_CHECKSUM_ADDR = 10;
  static const uint8_t EEPROM_V10_FLAG_100_VALID = 0x01;
  static const uint8_t EEPROM_V10_FLAG_300_VALID = 0x02;
  static const uint8_t EEPROM_V10_FLAG_500_VALID = 0x04;
  static const uint8_t EEPROM_V10_FLAGS_ALL_VALID = EEPROM_V10_FLAG_100_VALID |
                                                    EEPROM_V10_FLAG_300_VALID |
                                                    EEPROM_V10_FLAG_500_VALID;

  uint8_t magic0 = EEPROM.read(EEPROM_MAGIC0_ADDR);
  uint8_t magic1 = EEPROM.read(EEPROM_MAGIC1_ADDR);
  uint8_t version = EEPROM.read(EEPROM_VERSION_ADDR);
  uint16_t storedRaw100 = (uint16_t)EEPROM.read(EEPROM_RAW100_L_ADDR) |
                          ((uint16_t)EEPROM.read(EEPROM_RAW100_H_ADDR) << 8);
  uint16_t storedRaw300 = (uint16_t)EEPROM.read(EEPROM_V10_RAW300_L_ADDR) |
                          ((uint16_t)EEPROM.read(EEPROM_V10_RAW300_H_ADDR) << 8);
  uint16_t storedRaw500 = (uint16_t)EEPROM.read(EEPROM_V10_RAW500_L_ADDR) |
                          ((uint16_t)EEPROM.read(EEPROM_V10_RAW500_H_ADDR) << 8);
  uint8_t flags = EEPROM.read(EEPROM_V10_FLAGS_ADDR) & EEPROM_V10_FLAGS_ALL_VALID;
  uint8_t storedChecksum = EEPROM.read(EEPROM_V10_CHECKSUM_ADDR);

  if (magic0 != EEPROM_MAGIC0 || magic1 != EEPROM_MAGIC1 ||
      version != EEPROM_V10_VERSION ||
      storedChecksum != calibrationChecksumV10(storedRaw100, storedRaw300, storedRaw500, flags)) {
    calibrationValid = false;
    calibrationFlags = 0;
    return false;
  }

  if ((flags & EEPROM_V10_FLAG_100_VALID) != 0 &&
      calibrationPointValid(EEPROM_FLAG_100_VALID, storedRaw100)) {
    calibrationRaw100Mm = storedRaw100;
    calibrationFlags |= EEPROM_FLAG_100_VALID;
  }
  if ((flags & EEPROM_V10_FLAG_300_VALID) != 0 &&
      calibrationPointValid(EEPROM_FLAG_300_VALID, storedRaw300)) {
    calibrationRaw300Mm = storedRaw300;
    calibrationFlags |= EEPROM_FLAG_300_VALID;
  }
  if ((flags & EEPROM_V10_FLAG_500_VALID) != 0 &&
      calibrationPointValid(EEPROM_FLAG_500_VALID, storedRaw500)) {
    calibrationRaw500Mm = storedRaw500;
    calibrationFlags |= EEPROM_FLAG_500_VALID;
  }

  calibrationValid = calibrationFlags != 0;
  if (calibrationValid) {
    saveCalibration();
  }
  return calibrationValid;
}

void saveCalibration() {
  uint8_t flags = calibrationFlags & EEPROM_FLAGS_ALL_VALID;

  EEPROM.update(EEPROM_MAGIC0_ADDR, EEPROM_MAGIC0);
  EEPROM.update(EEPROM_MAGIC1_ADDR, EEPROM_MAGIC1);
  EEPROM.update(EEPROM_VERSION_ADDR, EEPROM_VERSION);
  EEPROM.update(EEPROM_RAW100_L_ADDR, (uint8_t)(calibrationRaw100Mm & 0xFF));
  EEPROM.update(EEPROM_RAW100_H_ADDR, (uint8_t)((calibrationRaw100Mm >> 8) & 0xFF));
  EEPROM.update(EEPROM_RAW200_L_ADDR, (uint8_t)(calibrationRaw200Mm & 0xFF));
  EEPROM.update(EEPROM_RAW200_H_ADDR, (uint8_t)((calibrationRaw200Mm >> 8) & 0xFF));
  EEPROM.update(EEPROM_RAW300_L_ADDR, (uint8_t)(calibrationRaw300Mm & 0xFF));
  EEPROM.update(EEPROM_RAW300_H_ADDR, (uint8_t)((calibrationRaw300Mm >> 8) & 0xFF));
  EEPROM.update(EEPROM_RAW400_L_ADDR, (uint8_t)(calibrationRaw400Mm & 0xFF));
  EEPROM.update(EEPROM_RAW400_H_ADDR, (uint8_t)((calibrationRaw400Mm >> 8) & 0xFF));
  EEPROM.update(EEPROM_RAW500_L_ADDR, (uint8_t)(calibrationRaw500Mm & 0xFF));
  EEPROM.update(EEPROM_RAW500_H_ADDR, (uint8_t)((calibrationRaw500Mm >> 8) & 0xFF));
  EEPROM.update(EEPROM_FLAGS_ADDR, flags);
  EEPROM.update(EEPROM_CHECKSUM_ADDR, calibrationChecksumV11(calibrationRaw100Mm,
                                                            calibrationRaw200Mm,
                                                            calibrationRaw300Mm,
                                                            calibrationRaw400Mm,
                                                            calibrationRaw500Mm,
                                                            flags));
}

uint8_t calibrationChecksumV11(uint16_t raw100Mm, uint16_t raw200Mm, uint16_t raw300Mm, uint16_t raw400Mm, uint16_t raw500Mm, uint8_t flags) {
  return EEPROM_MAGIC0 ^ EEPROM_MAGIC1 ^ EEPROM_VERSION ^
         (uint8_t)(raw100Mm & 0xFF) ^
         (uint8_t)((raw100Mm >> 8) & 0xFF) ^
         (uint8_t)(raw200Mm & 0xFF) ^
         (uint8_t)((raw200Mm >> 8) & 0xFF) ^
         (uint8_t)(raw300Mm & 0xFF) ^
         (uint8_t)((raw300Mm >> 8) & 0xFF) ^
         (uint8_t)(raw400Mm & 0xFF) ^
         (uint8_t)((raw400Mm >> 8) & 0xFF) ^
         (uint8_t)(raw500Mm & 0xFF) ^
         (uint8_t)((raw500Mm >> 8) & 0xFF) ^
         flags ^ 0x5A;
}

uint8_t calibrationChecksumV10(uint16_t raw100Mm, uint16_t raw300Mm, uint16_t raw500Mm, uint8_t flags) {
  static const uint8_t EEPROM_V10_VERSION = 10;

  return EEPROM_MAGIC0 ^ EEPROM_MAGIC1 ^ EEPROM_V10_VERSION ^
         (uint8_t)(raw100Mm & 0xFF) ^
         (uint8_t)((raw100Mm >> 8) & 0xFF) ^
         (uint8_t)(raw300Mm & 0xFF) ^
         (uint8_t)((raw300Mm >> 8) & 0xFF) ^
         (uint8_t)(raw500Mm & 0xFF) ^
         (uint8_t)((raw500Mm >> 8) & 0xFF) ^
         flags ^ 0x5A;
}
