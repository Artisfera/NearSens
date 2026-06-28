/*
  NearSens 1.0.0

  Firmware for prototype version v1.0.0. The code handles
  ultrasonic measurement, five-point 100-500 mm calibration, data storage
  in EEPROM, RAW mode, 75 mm correction and LCD codes.

  Version v1.0.0 was compiled for a 5 MHz clock.
  This file does not force the microcontroller clock. The clock setting
  is selected in Arduino IDE/megaTinyCore before compilation.
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
