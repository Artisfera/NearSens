/*
  NearSens firmware, v18 stable 3-point calibration.

  Hardware:
  - single TS3 / PA2 click: MEASURE button
  - five TS3 clicks: toggle RAW mode (7001 = on, 7000 = off)
  - short TS4 / PA1 click: toggle front/body reference (1_75 / 0_75 banner)
  - TS3 and TS4 held together for 5 s: start 100/300/500 mm calibration wizard
  - in the wizard, TS3 confirms the shown point and TS4 cancels
  - compile megaTinyCore for ATtiny1626 with the Clock selected in Arduino IDE
  - MAX232 and ADC are off in idle, then warmed up for the whole packet

  Measurement strategy:
  - first confirmed echo lobe, not strongest late peak
  - robust multi-ping packet for reliable echo capture
  - sort candidates, choose a tight distance cluster
  - full 100/300/500 mm fiveewise-linear calibration map with complete EEPROM v8 set
  - non-monotonic or out-of-window calibration is rejected instead of falling back
  - hold the final result until the next measurement

  RAW mode (five TS3 clicks):
  - bypasses applyCalibration() and body-reference offset
  - use to compare raw vs calibrated output for diagnostics

  If EEPROM calibration is missing or invalid, normal TS3 measurements fall
  back to raw distance so the prototype remains usable; RAW mode still bypasses calibration.

  The LCD shows a small BCD animation during measurement. BP/PH keeps running
  so the LCD does not receive DC bias.
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
