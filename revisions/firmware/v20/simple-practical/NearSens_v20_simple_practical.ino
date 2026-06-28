/*
  NearSens v20 practical firmware.

  Changes vs v18:
  - calibration wizard no longer forces all three points in one run;
    TS4 selects 0100 / 0300 / 0500 and TS3 calibrates only the shown point,
    then saves it to EEPROM.
  - partial calibration is allowed: one point gives offset correction,
    two points give a line, three points give 100/300/500 fiveewise correction.
  - measurement cluster selection uses the first stable distance cluster,
    not the largest later cluster. This avoids a repeatable false 300 mm lobe
    winning over a real nearer target.
  - no firmware clock lock. Select the CPU clock in megaTinyCore/Arduino IDE.

  Kept features:
  - TS3 measurement
  - TS4 front/body reference with DEVICE_REFERENCE_OFFSET_MM = 75
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
