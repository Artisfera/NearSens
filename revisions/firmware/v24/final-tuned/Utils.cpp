#include "NearSens.h"

uint16_t timeOfFlightToMm(uint32_t tofUs) {
  uint32_t mm = (tofUs * 343UL + 1000UL) / 2000UL;
  if (mm > LCD_DISPLAY_MAX) {
    return LCD_DISPLAY_MAX;
  }
  return (uint16_t)mm;
}

uint16_t absDiffAdc(uint16_t a, uint16_t b) {
  if (a > b) {
    return a - b;
  }
  return b - a;
}

uint16_t absDiff16(uint16_t a, uint16_t b) {
  if (a > b) {
    return a - b;
  }
  return b - a;
}

bool dueMs(uint32_t nowValue, uint32_t dueValue) {
  return (int32_t)(nowValue - dueValue) >= 0;
}

bool dueUs(uint32_t nowValue, uint32_t dueValue) {
  return (int32_t)(nowValue - dueValue) >= 0;
}
