/*
  NearSens firmware, strict 1 MHz v11 ready calibration fix.

  Hardware:
  - short TS3 / PA2 click: MEASURE button
  - short TS4 / PA1 click: switch front/body reference
  - TS3 and TS4 held together for 5 s: calibrate at 500 mm
  - single-button long holds are ignored intentionally to avoid old 100/300 calibration
  - compile megaTinyCore for ATtiny1626 with Clock = 1 MHz
  - MAX232 and ADC are off in idle, then warmed up for the whole packet

  Measurement strategy:
  - first confirmed echo lobe, not strongest late peak
  - robust multi-ping packet for reliable echo capture
  - sort candidates, choose a tight distance cluster
  - calibrated with one safe 500 mm EEPROM point as a global offset
  - hold the final result until the next measurement

  The LCD shows a small BCD animation during measurement. BP/PH keeps running
  so the LCD does not receive DC bias.
*/

#include "NearSens.h"

#if F_CPU != 1000000L
#warning "This NearSens firmware version is tuned for a 1 MHz CPU clock."
#endif

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

