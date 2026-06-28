#include "NearSens.h"

const uint8_t LCD_LD_MASKS[4] = {
  PIN_LCD_LD1_bm,
  PIN_LCD_LD2_bm,
  PIN_LCD_LD3_bm,
  PIN_LCD_LD4_bm
};

bool bpLevel = false;
bool measuringNow = false;
bool ts3WasPressed = false;
bool ts4WasPressed = false;
bool ts3HoldTriggered = false;
bool ts4HoldTriggered = false;
bool chordWasPressed = false;
bool chordHoldTriggered = false;
bool calibrationValid = false;
bool calibrationWizardActive = false;
bool lastMeasurementValid = false;
bool adcPowered = false;
bool deepSleeping = false;
bool rawMode = false;

uint32_t lastBpToggleUs = 0;
uint32_t startupUntilMs = 0;
uint32_t buttonLockoutUntilMs = 0;
uint32_t ts3HoldStartMs = 0;
uint32_t ts4HoldStartMs = 0;
uint32_t chordHoldStartMs = 0;
uint32_t animationLastMs = 0;
uint32_t bannerUntilMs = 0;
uint32_t lastUserActivityMs = 0;
uint32_t ts3ClickFirstMs = 0;

uint16_t displayValue = 8888;
uint16_t physicallyDisplayedValue = 0xFFFF;
uint8_t animationIndex = 0;
uint8_t displayMode = DISPLAY_IDLE;
uint8_t referenceMode = REFERENCE_FRONT;
uint8_t calibrationWizardStep = 0;
uint8_t calibrationFlags = 0;
uint8_t ts3ClickCount = 0;

uint16_t calibrationRaw100Mm = CAL_100_TARGET_MM;
uint16_t calibrationRaw300Mm = CAL_300_TARGET_MM;
uint16_t calibrationRaw500Mm = CAL_500_TARGET_MM;
uint16_t pendingCalibrationRaw100Mm = CAL_100_TARGET_MM;
uint16_t pendingCalibrationRaw300Mm = CAL_300_TARGET_MM;
uint16_t pendingCalibrationRaw500Mm = CAL_500_TARGET_MM;
uint16_t lastRawMeasurementMm = 0;
