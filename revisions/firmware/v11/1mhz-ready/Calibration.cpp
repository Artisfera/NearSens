#include <EEPROM.h>

#include "NearSens.h"

uint16_t applyCalibration(uint16_t rawMm) {
  /*
    Final v11 calibration: one safe 500 mm point.

    The previous 100/300/500 model could silently switch
    to the wrong segment or extrapolate outside the valid range when the 500 mm point
    was stored from a false echo near 300 mm.

    Here calibration works as a simple offset against the 500 mm point:
    corrected = raw + (500 - raw500).
    This does not flatten the 300-500 mm range and does not apply nonlinear correction
    from damaged point pairs.
  */
  int32_t corrected = rawMm;

  if (calibration500Loaded && calibrationRaw500Usable(calibrationRaw500Mm)) {
    corrected += (int32_t)CAL_500_TARGET_MM - (int32_t)calibrationRaw500Mm;
  }

  if (referenceMode == REFERENCE_BODY) {
    corrected += (int32_t)DEVICE_REFERENCE_OFFSET_MM;
  }

  if (corrected < 0) {
    return 0;
  }
  if (corrected > LCD_DISPLAY_MAX) {
    return LCD_DISPLAY_MAX;
  }
  return (uint16_t)corrected;
}

uint16_t applyCalibrationLine(uint16_t rawMm, uint16_t rawA, uint16_t targetA, uint16_t rawB, uint16_t targetB) {
  if (rawB <= rawA) {
    return rawMm;
  }

  uint16_t rawSpanU = rawB - rawA;
  if (rawSpanU < CAL_RAW_MIN_SEPARATION_MM) {
    return rawMm;
  }

  int32_t rawSpan = (int32_t)rawSpanU;
  int32_t targetSpan = (int32_t)targetB - (int32_t)targetA;
  int32_t numerator = ((int32_t)rawMm - (int32_t)rawA) * targetSpan;

  if (numerator >= 0) {
    numerator += rawSpan / 2;
  } else {
    numerator -= rawSpan / 2;
  }

  int32_t corrected = (int32_t)targetA + numerator / rawSpan;
  if (corrected < 0) {
    return 0;
  }
  if (corrected > LCD_DISPLAY_MAX) {
    return LCD_DISPLAY_MAX;
  }
  return (uint16_t)corrected;
}

bool calibrationPairUsable(uint16_t rawA, uint16_t rawB) {
  return rawB > (uint16_t)(rawA + CAL_RAW_MIN_SEPARATION_MM);
}

bool calibrationRaw500Usable(uint16_t rawMm) {
  return rawMm >= CAL_500_RAW_MIN_MM && rawMm <= CAL_500_RAW_MAX_MM;
}

void runCalibrationPoint(uint16_t targetMm) {
  // The final version allows only 500 mm calibration through TS3+TS4 held for 5 s.
  // Old 100/300 points are no longer stored, to avoid creating invalid segments
  // in the correction and effects such as 35 cm -> 30 cm.
  if (targetMm != CAL_500_TARGET_MM) {
    setDisplayValue(LCD_CAL_FAIL);
    return;
  }

  setDisplayValue(targetMm);
  serviceDisplay();

  uint32_t showUntilMs = millis() + 450UL;
  while (!dueMs(millis(), showUntilMs)) {
    serviceBackplane();
  }

  bool ok = false;
  uint16_t rawMm = measurePacket(RUN_CALIBRATE, &ok);

  if (!ok || !calibrationRaw500Usable(rawMm)) {
    setDisplayValue(LCD_CAL_FAIL);
    return;
  }

  calibrationRaw500Mm = rawMm;
  calibration500Loaded = true;
  calibration100Loaded = false;
  calibration300Loaded = false;
  calibrationRaw100Mm = CAL_100_TARGET_MM;
  calibrationRaw300Mm = CAL_300_TARGET_MM;

  saveCalibration();
  lastMeasurementValid = false;
  setDisplayValue(CAL_500_TARGET_MM);
}

void loadCalibration() {
  referenceMode = REFERENCE_FRONT;
  calibrationRaw100Mm = CAL_100_TARGET_MM;
  calibrationRaw300Mm = CAL_300_TARGET_MM;
  calibrationRaw500Mm = CAL_500_TARGET_MM;
  calibration100Loaded = false;
  calibration300Loaded = false;
  calibration500Loaded = false;

  (void)loadCalibrationV5();
}

bool loadCalibrationV5() {
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
      storedChecksum != calibrationChecksumV5(storedRaw100, storedRaw300,
                                              storedRaw500, flags)) {
    return false;
  }

  if ((flags & EEPROM_FLAG_500_VALID) == 0 || !calibrationRaw500Usable(storedRaw500)) {
    return false;
  }

  calibrationRaw100Mm = CAL_100_TARGET_MM;
  calibrationRaw300Mm = CAL_300_TARGET_MM;
  calibrationRaw500Mm = storedRaw500;
  calibration100Loaded = false;
  calibration300Loaded = false;
  calibration500Loaded = true;
  return true;
}

bool loadCalibrationV4() {
  return false;
}

bool loadCalibrationV3() {
  return false;
}

void saveCalibration() {
  uint8_t flags = 0;
  uint16_t raw100 = CAL_100_TARGET_MM;
  uint16_t raw300 = CAL_300_TARGET_MM;
  uint16_t raw500 = CAL_500_TARGET_MM;

  if (calibration500Loaded && calibrationRaw500Usable(calibrationRaw500Mm)) {
    flags |= EEPROM_FLAG_500_VALID;
    raw500 = calibrationRaw500Mm;
  }

  EEPROM.update(EEPROM_MAGIC0_ADDR, EEPROM_MAGIC0);
  EEPROM.update(EEPROM_MAGIC1_ADDR, EEPROM_MAGIC1);
  EEPROM.update(EEPROM_VERSION_ADDR, EEPROM_VERSION);
  EEPROM.update(EEPROM_RAW100_L_ADDR, (uint8_t)(raw100 & 0xFF));
  EEPROM.update(EEPROM_RAW100_H_ADDR, (uint8_t)((raw100 >> 8) & 0xFF));
  EEPROM.update(EEPROM_RAW300_L_ADDR, (uint8_t)(raw300 & 0xFF));
  EEPROM.update(EEPROM_RAW300_H_ADDR, (uint8_t)((raw300 >> 8) & 0xFF));
  EEPROM.update(EEPROM_RAW500_L_ADDR, (uint8_t)(raw500 & 0xFF));
  EEPROM.update(EEPROM_RAW500_H_ADDR, (uint8_t)((raw500 >> 8) & 0xFF));
  EEPROM.update(EEPROM_FLAGS_ADDR, flags);
  EEPROM.update(EEPROM_CHECKSUM_ADDR, calibrationChecksumV5(raw100, raw300, raw500, flags));
}

uint8_t calibrationChecksumV5(uint16_t raw100Mm, uint16_t raw300Mm, uint16_t raw500Mm, uint8_t flags) {
  return EEPROM_MAGIC0 ^ EEPROM_MAGIC1 ^ EEPROM_VERSION ^
         (uint8_t)(raw100Mm & 0xFF) ^
         (uint8_t)((raw100Mm >> 8) & 0xFF) ^
         (uint8_t)(raw300Mm & 0xFF) ^
         (uint8_t)((raw300Mm >> 8) & 0xFF) ^
         (uint8_t)(raw500Mm & 0xFF) ^
         (uint8_t)((raw500Mm >> 8) & 0xFF) ^
         flags ^ 0x5A;
}

uint8_t calibrationChecksumV4(uint16_t raw500Mm, uint8_t flags) {
  return EEPROM_MAGIC0 ^ EEPROM_MAGIC1 ^ 4 ^
         (uint8_t)(raw500Mm & 0xFF) ^
         (uint8_t)((raw500Mm >> 8) & 0xFF) ^
         flags ^ 0x5A;
}

uint8_t calibrationChecksumV3(uint16_t raw100Mm, uint16_t raw300Mm, uint8_t flags) {
  return EEPROM_MAGIC0 ^ EEPROM_MAGIC1 ^ 3 ^
         (uint8_t)(raw100Mm & 0xFF) ^
         (uint8_t)((raw100Mm >> 8) & 0xFF) ^
         (uint8_t)(raw300Mm & 0xFF) ^
         (uint8_t)((raw300Mm >> 8) & 0xFF) ^
         flags ^ 0x5A;
}
