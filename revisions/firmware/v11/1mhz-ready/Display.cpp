#include "NearSens.h"

void serviceBackplane() {
  uint32_t nowUs = micros();
  serviceBackplaneAt(nowUs);
}

void serviceBackplaneAt(uint32_t nowUs) {
  if (!dueUs(nowUs, lastBpToggleUs + LCD_BP_TOGGLE_US)) {
    return;
  }

  lastBpToggleUs = nowUs;
  bpLevel = !bpLevel;
  if (bpLevel) {
    VPORTB.OUT |= PIN_LCD_BP_bm;
  } else {
    VPORTB.OUT &= ~PIN_LCD_BP_bm;
  }
}

void serviceDisplay() {
  if (measuringNow) {
    return;
  }

  if (displayMode == DISPLAY_BANNER) {
    if (dueMs(millis(), bannerUntilMs)) {
      if (lastMeasurementValid) {
        setDisplayValue(applyCalibration(lastRawMeasurementMm));
      } else {
        showIdleMarker();
      }
    }
    return;
  }

  if (displayMode == DISPLAY_IDLE) {
    return;
  }

  if (displayValue != physicallyDisplayedValue) {
    lcdShowValue(displayValue);
    physicallyDisplayedValue = displayValue;
  }
}

void startMeasureAnimation() {
  measuringNow = true;
  lcdSetBlank(false);
  animationIndex = 0;
  animationLastMs = 0;
  physicallyDisplayedValue = 0xFFFF;
  serviceMeasureAnimation();
}

void stopMeasureAnimation() {
  measuringNow = false;
  physicallyDisplayedValue = 0xFFFF;
}

void serviceMeasureAnimation() {
  if (!measuringNow) {
    return;
  }

  uint32_t nowMs = millis();
  if (animationLastMs != 0 && !dueMs(nowMs, animationLastMs + 65UL)) {
    return;
  }

  static const uint16_t patterns[8] = {
    8000, 800, 80, 8, 8, 80, 800, 8000
  };

  animationLastMs = nowMs;
  lcdShowValue(patterns[animationIndex & 0x07]);
  physicallyDisplayedValue = patterns[animationIndex & 0x07];
  animationIndex++;
}

void setDisplayValue(uint16_t value) {
  lcdSetBlank(false);
  displayValue = value;
  displayMode = DISPLAY_VALUE;
  physicallyDisplayedValue = 0xFFFF;
}

void showIdleMarker() {
  lcdSetBlank(false);
  displayMode = DISPLAY_IDLE;
  lcdShowValue(8888);
  physicallyDisplayedValue = 8888;
}

void showReferenceBanner() {
  lcdSetBlank(false);
  displayMode = DISPLAY_BANNER;
  bannerUntilMs = millis() + REFERENCE_BANNER_MS;
  if (referenceMode == REFERENCE_BODY) {
    lcdShowRawBcd4(1, LCD_RAW_BLANK, 7, 5);
  } else {
    lcdShowRawBcd4(0, LCD_RAW_BLANK, 7, 5);
  }
  physicallyDisplayedValue = 0xFFFF;
}

void lcdSetBlank(bool blank) {
  if (blank) {
    VPORTB.OUT |= PIN_LCD_BL_bm;
  } else {
    VPORTB.OUT &= ~PIN_LCD_BL_bm;
  }
}

void lcdShowValue(uint16_t value) {
  if (value > LCD_DISPLAY_MAX) {
    value = LCD_DISPLAY_MAX;
  }

  lcdWriteDigit(0, (value / 1000) % 10);
  lcdWriteDigit(1, (value / 100) % 10);
  lcdWriteDigit(2, (value / 10) % 10);
  lcdWriteDigit(3, value % 10);
}

void lcdShowRawBcd4(uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3) {
  lcdWriteDigit(0, d0);
  lcdWriteDigit(1, d1);
  lcdWriteDigit(2, d2);
  lcdWriteDigit(3, d3);
}

void lcdWriteDigit(uint8_t digitIndex, uint8_t bcd) {
  lcdSetBcd(bcd);
  lcdPulseLatch(digitIndex);
}

void lcdSetBcd(uint8_t bcd) {
  uint8_t outValue = VPORTC.OUT & ~LCD_BCD_MASK;

  if (bcd & 0x01) {
    outValue |= PIN_LCD_A_bm;
  }
  if (bcd & 0x02) {
    outValue |= PIN_LCD_B_bm;
  }
  if (bcd & 0x04) {
    outValue |= PIN_LCD_C_bm;
  }
  if (bcd & 0x08) {
    outValue |= PIN_LCD_D_bm;
  }

  VPORTC.OUT = outValue;
}

void lcdPulseLatch(uint8_t digitIndex) {
  uint8_t mask = LCD_LD_MASKS[digitIndex & 0x03];
  VPORTB.OUT |= mask;
  delayMicroseconds(LCD_LATCH_PULSE_US);
  VPORTB.OUT &= ~mask;
}
