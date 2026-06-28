/*
  NearSens v24 1MHz final firmware.

  Target: ATtiny1626, megaTinyCore, CPU clock selected as 1 MHz.

  Main fixes vs v23:
  - whole firmware package, not only NearSens.h,
  - faster explicit ADC setup for 1 MHz operation,
  - target-aware calibration clustering for 100/300/500 mm,
  - EEPROM version bumped, so old bad calibration is ignored,
  - TS4 75 mm offset still applies only to normal displayed result,
  - RAW mode still shows uncalibrated distance from sensor front.

  Important: firmware improves repeatability, but the final accuracy still depends
  on the analog RX path, transducers, target, supply noise and calibration setup.
*/

#include "NearSens.h"


void setup() {
  configurePins();
  configureAdc();
  loadCalibration();
  setMax232Power(false);
  setAdcPower(false);

  uint32_t nowMs = millis();
  startupUntilMs = nowMs + STARTUP_DISPLAY_MS;
  buttonLockoutUntilMs = startupUntilMs;
  lastUserActivityMs = nowMs;
  displayMode = DISPLAY_IDLE;

  lcdShowValue(8888);
  physicallyDisplayedValue = 8888;
}

void loop() {
  serviceBackplane();
  serviceMeasureAnimation();
  serviceButtons();
  serviceDisplay();
  serviceDeepSleep();
  idleUntilInterrupt();
}
