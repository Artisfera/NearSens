/*
  NearSens v22 offset fix firmware.

  Changes vs v20:
  - measureOnePing now scans the whole echo window and picks the strongest
    qualified pulse instead of returning on the first threshold crossing.
  - BP/PH service is paused only during the short ADC echo scan to reduce
    digital switching noise in the RX window.
  - tighter measurement/calibration clusters for better repeatability.
  - timeout is extended enough for 50 cm plus analog delay, without opening
    the old 1.5 m secondary-reflection window.
  - no firmware clock lock. Select the CPU clock in megaTinyCore/Arduino IDE.

  Kept features:
  - TS3 measurement
  - TS4 front/body reference; BODY subtracts DEVICE_REFERENCE_OFFSET_MM = 75 exactly once
  - RAW mode by five TS3 clicks
  - idle 8888
  - 40xx error codes
  - BP/PH LCD service
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
