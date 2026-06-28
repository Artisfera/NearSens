#include <EEPROM.h>

#include "NearSens.h"

static uint16_t calibrationTargetForStep(uint8_t step) {
  if (step == 0) {
    return CAL_100_TARGET_MM;
  }
  if (step == 1) {
    return CAL_300_TARGET_MM;
  }
  return CAL_500_TARGET_MM;
}

static uint8_t calibrationFlagForTarget(uint16_t targetMm) {
  if (targetMm == CAL_100_TARGET_MM) {
    return EEPROM_FLAG_100_VALID;
  }
  if (targetMm == CAL_300_TARGET_MM) {
    return EEPROM_FLAG_300_VALID;
  }
  return EEPROM_FLAG_500_VALID;
}

static uint16_t rawForTarget(uint16_t targetMm) {
  if (targetMm == CAL_100_TARGET_MM) {
    return calibrationRaw100Mm;
  }
  if (targetMm == CAL_300_TARGET_MM) {
    return calibrationRaw300Mm;
  }
  return calibrationRaw500Mm;
}

static uint16_t targetForFlag(uint8_t flag) {
  if (flag == EEPROM_FLAG_100_VALID) {
    return CAL_100_TARGET_MM;
  }
  if (flag == EEPROM_FLAG_300_VALID) {
    return CAL_300_TARGET_MM;
  }
  return CAL_500_TARGET_MM;
}

bool applyCalibration(uint16_t rawMm, uint16_t *correctedMm) {
  if (correctedMm == 0) {
    return false;
  }

  uint16_t calibratedMm = rawMm;
  bool ok = false;
  uint8_t flags = calibrationFlags;

  bool has100 = (flags & EEPROM_FLAG_100_VALID) != 0 &&
                calibrationPointValid(EEPROM_FLAG_100_VALID, calibrationRaw100Mm);
  bool has300 = (flags & EEPROM_FLAG_300_VALID) != 0 &&
                calibrationPointValid(EEPROM_FLAG_300_VALID, calibrationRaw300Mm);
  bool has500 = (flags & EEPROM_FLAG_500_VALID) != 0 &&
                calibrationPointValid(EEPROM_FLAG_500_VALID, calibrationRaw500Mm);

  /*
    Prefer the local calibrated segment. This keeps the important 300 mm point
    useful without forcing every calibration session to collect all three points.
  */
  if (has100 && has300 && rawMm <= calibrationRaw300Mm &&
      calibrationPairValid(calibrationRaw100Mm, calibrationRaw300Mm,
                           CAL_RAW_MIN_SEPARATION_100_300_MM)) {
    ok = interpolateCalibratedValue(rawMm,
                                    calibrationRaw100Mm, CAL_100_TARGET_MM,
                                    calibrationRaw300Mm, CAL_300_TARGET_MM,
                                    &calibratedMm);
  } else if (has300 && has500 && rawMm >= calibrationRaw300Mm &&
             calibrationPairValid(calibrationRaw300Mm, calibrationRaw500Mm,
                                  CAL_RAW_MIN_SEPARATION_300_500_MM)) {
    ok = interpolateCalibratedValue(rawMm,
                                    calibrationRaw300Mm, CAL_300_TARGET_MM,
                                    calibrationRaw500Mm, CAL_500_TARGET_MM,
                                    &calibratedMm);
  } else if (has100 && has500 &&
             calibrationPairValid(calibrationRaw100Mm, calibrationRaw500Mm,
                                  CAL_RAW_MIN_SEPARATION_100_300_MM +
                                  CAL_RAW_MIN_SEPARATION_300_500_MM)) {
    ok = interpolateCalibratedValue(rawMm,
                                    calibrationRaw100Mm, CAL_100_TARGET_MM,
                                    calibrationRaw500Mm, CAL_500_TARGET_MM,
                                    &calibratedMm);
  } else if (has100 && has300 &&
             calibrationPairValid(calibrationRaw100Mm, calibrationRaw300Mm,
                                  CAL_RAW_MIN_SEPARATION_100_300_MM)) {
    ok = interpolateCalibratedValue(rawMm,
                                    calibrationRaw100Mm, CAL_100_TARGET_MM,
                                    calibrationRaw300Mm, CAL_300_TARGET_MM,
                                    &calibratedMm);
  } else if (has300 && has500 &&
             calibrationPairValid(calibrationRaw300Mm, calibrationRaw500Mm,
                                  CAL_RAW_MIN_SEPARATION_300_500_MM)) {
    ok = interpolateCalibratedValue(rawMm,
                                    calibrationRaw300Mm, CAL_300_TARGET_MM,
                                    calibrationRaw500Mm, CAL_500_TARGET_MM,
                                    &calibratedMm);
  } else if (has300) {
    int32_t corrected = (int32_t)rawMm +
                        ((int32_t)CAL_300_TARGET_MM - (int32_t)calibrationRaw300Mm);
    if (corrected < 0) {
      calibratedMm = 0;
    } else if (corrected > LCD_DISPLAY_MAX) {
      calibratedMm = LCD_DISPLAY_MAX;
    } else {
      calibratedMm = (uint16_t)corrected;
    }
    ok = true;
  } else if (has100) {
    int32_t corrected = (int32_t)rawMm +
                        ((int32_t)CAL_100_TARGET_MM - (int32_t)calibrationRaw100Mm);
    if (corrected < 0) {
      calibratedMm = 0;
    } else if (corrected > LCD_DISPLAY_MAX) {
      calibratedMm = LCD_DISPLAY_MAX;
    } else {
      calibratedMm = (uint16_t)corrected;
    }
    ok = true;
  } else if (has500) {
    int32_t corrected = (int32_t)rawMm +
                        ((int32_t)CAL_500_TARGET_MM - (int32_t)calibrationRaw500Mm);
    if (corrected < 0) {
      calibratedMm = 0;
    } else if (corrected > LCD_DISPLAY_MAX) {
      calibratedMm = LCD_DISPLAY_MAX;
    } else {
      calibratedMm = (uint16_t)corrected;
    }
    ok = true;
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

bool calibrationFullSetValid(uint16_t raw100Mm, uint16_t raw300Mm, uint16_t raw500Mm) {
  if (!calibrationPointValid(EEPROM_FLAG_100_VALID, raw100Mm) ||
      !calibrationPointValid(EEPROM_FLAG_300_VALID, raw300Mm) ||
      !calibrationPointValid(EEPROM_FLAG_500_VALID, raw500Mm)) {
    return false;
  }
  if (!calibrationPairValid(raw100Mm, raw300Mm, CAL_RAW_MIN_SEPARATION_100_300_MM)) {
    return false;
  }
  if (!calibrationPairValid(raw300Mm, raw500Mm, CAL_RAW_MIN_SEPARATION_300_500_MM)) {
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
  if (flag == EEPROM_FLAG_300_VALID) {
    return calibrationRawInWindow(rawMm, CAL_VALID_RAW300_MIN_MM, CAL_VALID_RAW300_MAX_MM);
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
  calibrationWizardStep = 1;  // Default to the most useful correction point, 300 mm.
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
  if (calibrationWizardStep > 2) {
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

  if (targetMm == CAL_100_TARGET_MM) {
    calibrationRaw100Mm = rawMm;
  } else if (targetMm == CAL_300_TARGET_MM) {
    calibrationRaw300Mm = rawMm;
  } else {
    calibrationRaw500Mm = rawMm;
  }

  calibrationFlags |= pointFlag;

  /*
    If all three points exist but are inconsistent, keep the freshly calibrated
    point and drop only the conflicting neighbours. This prevents the old 4004
    trap where one bad old EEPROM point made a good new point unusable.
  */
  if ((calibrationFlags & EEPROM_FLAGS_ALL_VALID) == EEPROM_FLAGS_ALL_VALID &&
      !calibrationFullSetValid(calibrationRaw100Mm, calibrationRaw300Mm, calibrationRaw500Mm)) {
    if (pointFlag == EEPROM_FLAG_100_VALID) {
      calibrationFlags &= (uint8_t)~(EEPROM_FLAG_300_VALID | EEPROM_FLAG_500_VALID);
    } else if (pointFlag == EEPROM_FLAG_300_VALID) {
      calibrationFlags &= (uint8_t)~(EEPROM_FLAG_100_VALID | EEPROM_FLAG_500_VALID);
    } else {
      calibrationFlags &= (uint8_t)~(EEPROM_FLAG_100_VALID | EEPROM_FLAG_300_VALID);
    }
  }

  calibrationValid = calibrationFlags != 0;
  saveCalibration();
  lastMeasurementValid = false;
  calibrationWizardActive = false;
  setDisplayValue(LCD_CAL_SUCCESS);
}

static bool chooseTargetedCalibrationCluster(uint16_t *values,
                                             uint8_t count,
                                             uint16_t targetMm,
                                             uint16_t clusterMm,
                                             uint8_t minCluster,
                                             uint16_t *resultMm) {
  if (values == 0 || resultMm == 0 || count < minCluster) {
    return false;
  }

  sortValues(values, count);

  bool found = false;
  uint8_t bestStart = 0;
  uint8_t bestEnd = 0;
  uint8_t bestMembers = 0;
  uint16_t bestMedian = 0;
  uint16_t bestDistanceToTarget = 0xFFFF;
  uint16_t bestSpread = 0xFFFF;

  for (uint8_t start = 0; start < count; start++) {
    uint8_t end = start;
    while ((uint8_t)(end + 1) < count &&
           (uint16_t)(values[end + 1] - values[start]) <= clusterMm) {
      end++;
    }

    uint8_t members = end - start + 1;
    if (members < minCluster) {
      continue;
    }

    uint8_t middle = start + (members / 2);
    uint16_t median = 0;
    if ((members & 0x01) != 0) {
      median = values[middle];
    } else {
      median = (uint16_t)(((uint32_t)values[middle - 1] + values[middle] + 1UL) / 2UL);
    }

    uint16_t distanceToTarget = absDiff16(median, targetMm);
    uint16_t spread = values[end] - values[start];

    bool better = false;
    if (!found) {
      better = true;
    } else if (distanceToTarget < bestDistanceToTarget) {
      better = true;
    } else if (distanceToTarget == bestDistanceToTarget && members > bestMembers) {
      better = true;
    } else if (distanceToTarget == bestDistanceToTarget && members == bestMembers && spread < bestSpread) {
      better = true;
    }

    if (better) {
      found = true;
      bestStart = start;
      bestEnd = end;
      bestMembers = members;
      bestMedian = median;
      bestDistanceToTarget = distanceToTarget;
      bestSpread = spread;
    }
  }

  if (!found) {
    return false;
  }

  (void)bestStart;
  (void)bestEnd;
  *resultMm = bestMedian;
  return true;
}

bool captureCalibrationPoint(uint16_t targetMm, uint16_t *rawMm, uint16_t *errorCode) {
  if (rawMm == 0 || errorCode == 0) {
    return false;
  }

  uint16_t samples[CAL_POINT_SAMPLES];
  uint8_t goodCount = 0;
  uint8_t rxFaultCount = 0;
  uint8_t stuckCount = 0;

  /*
    1 MHz calibration mode: do not run nested measurePacket() calls here.
    The older code performed several packet-level measurements and then rejected
    the whole point when one packet landed outside the spread window. At 1 MHz
    that made calibration fail even when many single pings were useful.

    This routine powers the analog path once, takes many single pings, ignores
    outliers, and stores the stable cluster closest to the requested calibration
    target (100/300/500 mm). That is both simpler and more reliable for this PCB.
  */
  setDisplayValue(targetMm);
  serviceDisplay();
  prepareMeasurementPower();

  for (uint8_t i = 0; i < CAL_POINT_SAMPLES; i++) {
    bool ok = false;
    uint16_t sample = measureOnePing(&ok);

    if (ok && goodCount < CAL_POINT_SAMPLES) {
      samples[goodCount++] = sample;
    } else if (sample == LCD_RX_FAULT) {
      rxFaultCount++;
    } else if (sample == LCD_ECHO_STUCK) {
      stuckCount++;
    }

    uint32_t waitUntilMs = millis() + INTER_PING_MS;
    while (!dueMs(millis(), waitUntilMs)) {
      serviceBackplane();
    }
  }

  finishMeasurementPower();

  if (goodCount < CAL_POINT_MIN_GOOD) {
    if (rxFaultCount >= 3) {
      *errorCode = LCD_RX_FAULT;
    } else if (stuckCount >= 3) {
      *errorCode = LCD_ECHO_STUCK;
    } else {
      *errorCode = LCD_CAL_FAIL;
    }
    return false;
  }

  uint16_t selectedRawMm = 0;
  bool selected = chooseTargetedCalibrationCluster(samples,
                                                  goodCount,
                                                  targetMm,
                                                  CAL_POINT_MAX_SPREAD_MM,
                                                  CAL_POINT_MIN_GOOD,
                                                  &selectedRawMm);

  // 1 MHz safety fallback: if the strict cluster is just too demanding, try a
  // wider cluster with fewer members instead of throwing 4006 immediately.
  // This still rejects random garbage, but avoids failing good calibrations
  // because a few pings missed the echo.
  if (!selected && goodCount >= 5) {
    selected = chooseTargetedCalibrationCluster(samples,
                                                goodCount,
                                                targetMm,
                                                38,
                                                5,
                                                &selectedRawMm);
  }

  if (!selected) {
    *errorCode = LCD_CAL_SPREAD;
    return false;
  }

  *rawMm = selectedRawMm;
  return true;
}

void loadCalibration() {
  referenceMode = REFERENCE_FRONT;
  rawMode = false;
  calibrationRaw100Mm = CAL_100_TARGET_MM;
  calibrationRaw300Mm = CAL_300_TARGET_MM;
  calibrationRaw500Mm = CAL_500_TARGET_MM;
  calibrationFlags = 0;
  calibrationValid = false;

  (void)loadCalibrationV10();
}

bool loadCalibrationV10() {
  uint8_t magic0 = EEPROM.read(EEPROM_MAGIC0_ADDR);
  uint8_t magic1 = EEPROM.read(EEPROM_MAGIC1_ADDR);
  uint8_t version = EEPROM.read(EEPROM_VERSION_ADDR);
  uint16_t storedRaw100 = (uint16_t)EEPROM.read(EEPROM_RAW100_L_ADDR) |
                          ((uint16_t)EEPROM.read(EEPROM_RAW100_H_ADDR) << 8);
  uint16_t storedRaw300 = (uint16_t)EEPROM.read(EEPROM_RAW300_L_ADDR) |
                          ((uint16_t)EEPROM.read(EEPROM_RAW300_H_ADDR) << 8);
  uint16_t storedRaw500 = (uint16_t)EEPROM.read(EEPROM_RAW500_L_ADDR) |
                          ((uint16_t)EEPROM.read(EEPROM_RAW500_H_ADDR) << 8);
  uint8_t flags = EEPROM.read(EEPROM_FLAGS_ADDR) & EEPROM_FLAGS_ALL_VALID;
  uint8_t storedChecksum = EEPROM.read(EEPROM_CHECKSUM_ADDR);

  if (magic0 != EEPROM_MAGIC0 || magic1 != EEPROM_MAGIC1 ||
      version != EEPROM_VERSION ||
      storedChecksum != calibrationChecksumV10(storedRaw100, storedRaw300, storedRaw500, flags)) {
    calibrationValid = false;
    calibrationFlags = 0;
    return false;
  }

  if ((flags & EEPROM_FLAG_100_VALID) != 0 &&
      calibrationPointValid(EEPROM_FLAG_100_VALID, storedRaw100)) {
    calibrationRaw100Mm = storedRaw100;
    calibrationFlags |= EEPROM_FLAG_100_VALID;
  }
  if ((flags & EEPROM_FLAG_300_VALID) != 0 &&
      calibrationPointValid(EEPROM_FLAG_300_VALID, storedRaw300)) {
    calibrationRaw300Mm = storedRaw300;
    calibrationFlags |= EEPROM_FLAG_300_VALID;
  }
  if ((flags & EEPROM_FLAG_500_VALID) != 0 &&
      calibrationPointValid(EEPROM_FLAG_500_VALID, storedRaw500)) {
    calibrationRaw500Mm = storedRaw500;
    calibrationFlags |= EEPROM_FLAG_500_VALID;
  }

  calibrationValid = calibrationFlags != 0;
  return calibrationValid;
}

void saveCalibration() {
  uint8_t flags = calibrationFlags & EEPROM_FLAGS_ALL_VALID;

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
  EEPROM.update(EEPROM_CHECKSUM_ADDR, calibrationChecksumV10(calibrationRaw100Mm,
                                                            calibrationRaw300Mm,
                                                            calibrationRaw500Mm,
                                                            flags));
}

uint8_t calibrationChecksumV10(uint16_t raw100Mm, uint16_t raw300Mm, uint16_t raw500Mm, uint8_t flags) {
  return EEPROM_MAGIC0 ^ EEPROM_MAGIC1 ^ EEPROM_VERSION ^
         (uint8_t)(raw100Mm & 0xFF) ^
         (uint8_t)((raw100Mm >> 8) & 0xFF) ^
         (uint8_t)(raw300Mm & 0xFF) ^
         (uint8_t)((raw300Mm >> 8) & 0xFF) ^
         (uint8_t)(raw500Mm & 0xFF) ^
         (uint8_t)((raw500Mm >> 8) & 0xFF) ^
         flags ^ 0x5A;
}
