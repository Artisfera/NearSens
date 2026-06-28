/*
  NearSens v30 from v23, 1 MHz stable candidate.

  Base: v23 offset mode fix, because that was the last hardware-good branch.

  1 MHz changes:
  - more repeated pings instead of rewriting the whole algorithm,
  - real far-threshold use after FAR_ECHO_START_MM,
  - far echo can pass with one sampled hit, then packet clustering validates it,
  - calibration uses fewer packet-level repeats, but each packet has many pings,
  - no firstUs timing change from v24, peakUs timing is kept,
  - no firmware clock lock. Select CPU clock in megaTinyCore/Arduino IDE.

  Kept features:
  - TS3 measurement,
  - TS4 toggles 0 _ 75 / 1 _ 75 offset display mode,
  - RAW mode by five TS3 clicks,
  - selectable calibration point with TS4,
  - idle 8888, 40xx error codes, BP/PH LCD service.
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
