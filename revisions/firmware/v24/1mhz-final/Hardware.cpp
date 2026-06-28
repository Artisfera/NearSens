#include "NearSens.h"

#include <avr/interrupt.h>

ISR(PORTA_PORT_vect) {
  PORTA.INTFLAGS = PIN_TS3_bm | PIN_TS4_bm;
}

void configurePins() {
  VPORTA.OUT &= ~(PIN_TX_T1_bm | PIN_TX_T2_bm);
  VPORTA.OUT |= PIN_MAX_ACTIVE_bm;
  VPORTB.OUT &= ~(PIN_LCD_BP_bm | PIN_LCD_LD1_bm | PIN_LCD_LD2_bm |
                  PIN_LCD_LD3_bm | PIN_LCD_LD4_bm | PIN_LCD_BL_bm);
  VPORTC.OUT &= ~LCD_BCD_MASK;

  PORTA.DIRSET = PIN_TX_T1_bm | PIN_TX_T2_bm | PIN_MAX_ACTIVE_bm;
  PORTA.DIRCLR = PIN_ECHO_bm | PIN_TS3_bm | PIN_TS4_bm;
  PORTB.DIRSET = PIN_LCD_BP_bm | PIN_LCD_LD1_bm | PIN_LCD_LD2_bm |
                 PIN_LCD_LD3_bm | PIN_LCD_LD4_bm | PIN_LCD_BL_bm;
  PORTC.DIRSET = LCD_BCD_MASK;

  PORTA.PIN1CTRL = PORT_PULLUPEN_bm;
  PORTA.PIN2CTRL = PORT_PULLUPEN_bm;
  PORTA.PIN6CTRL = PORT_ISC_INPUT_DISABLE_gc;
}

void configureAdc() {
  // Direct ADC setup is used here instead of Arduino analogRead setup, because
  // the RX window is short and the 1 MHz CPU clock leaves little timing margin.
  // ADCn.CTRLB PRESC = 0 means CLK_ADC = CLK_PER / 2.
  // ADCn.CTRLC REFSEL = 0 means VDD reference. TIMEBASE is set to the number
  // of CLK_PER cycles needed for at least 1 us.
  uint8_t adcTimebase = (uint8_t)((F_CPU + 999999UL) / 1000000UL);
  if (adcTimebase < 1) {
    adcTimebase = 1;
  }
  if (adcTimebase > 31) {
    adcTimebase = 31;
  }

  ADC0.CTRLA = 0;
  ADC0.CTRLB = 0x00;
  ADC0.CTRLC = (uint8_t)(adcTimebase << 3);
  ADC0.CTRLE = 0x00;
  ADC0.CTRLF = 0x00;
  ADC0.COMMAND = 0x00;
  ADC0.MUXPOS = ECHO_ADC_MUXPOS;

  setAdcPower(true);
  for (uint8_t i = 0; i < ADC_WARMUP_READS; i++) {
    (void)readEchoAdc();
  }
  setAdcPower(false);
}

void setAdcPower(bool on) {
  if (on) {
    ADC0.MUXPOS = ECHO_ADC_MUXPOS;
    ADC0.CTRLA |= (ADC_ENABLE_bm | ADC_LOWLAT_bm);
    adcPowered = true;
  } else {
    while (ADC0.COMMAND & ADC_START_gm) {
    }
    ADC0.CTRLA &= ~ADC_ENABLE_bm;
    adcPowered = false;
  }
}

void prepareMeasurementPower() {
  setMax232Power(true);
  setAdcPower(true);

  uint32_t settleUntilMs = millis() + MEASURE_POWER_SETTLE_MS;
  while (!dueMs(millis(), settleUntilMs)) {
    serviceBackplane();
  }

  for (uint8_t i = 0; i < ADC_WARMUP_READS; i++) {
    (void)readEchoAdc();
    serviceBackplane();
    delayMicroseconds(40);
  }
}

void finishMeasurementPower() {
  VPORTA.OUT &= ~(PIN_TX_T1_bm | PIN_TX_T2_bm);
  setAdcPower(false);
  setMax232Power(false);
}

void idleUntilInterrupt() {
  if (measuringNow) {
    return;
  }

  set_sleep_mode(SLEEP_MODE_IDLE);
  sleep_enable();
  sleep_cpu();
  sleep_disable();
}

void serviceDeepSleep() {
  if (measuringNow || displayMode == DISPLAY_BANNER ||
      calibrationWizardActive || ts3ClickCount != 0) {
    return;
  }
  if (!dueMs(millis(), startupUntilMs) || !dueMs(millis(), buttonLockoutUntilMs)) {
    return;
  }
  if (dueMs(millis(), lastUserActivityMs + DEEP_SLEEP_TIMEOUT_MS)) {
    enterDeepSleep();
  }
}

void enterDeepSleep() {
  deepSleeping = true;
  VPORTA.OUT &= ~(PIN_TX_T1_bm | PIN_TX_T2_bm);
  setAdcPower(false);
  setMax232Power(false);
  lcdSetBlank(true);
  VPORTB.OUT &= ~PIN_LCD_BP_bm;
  bpLevel = false;

  PORTA.INTFLAGS = PIN_TS3_bm | PIN_TS4_bm;
  PORTA.PIN1CTRL = PORT_PULLUPEN_bm | PORT_ISC_LEVEL_gc;
  PORTA.PIN2CTRL = PORT_PULLUPEN_bm | PORT_ISC_LEVEL_gc;

  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  sleep_cpu();
  sleep_disable();

  wakeFromDeepSleep();
}

void wakeFromDeepSleep() {
  PORTA.PIN1CTRL = PORT_PULLUPEN_bm;
  PORTA.PIN2CTRL = PORT_PULLUPEN_bm;
  PORTA.INTFLAGS = PIN_TS3_bm | PIN_TS4_bm;

  deepSleeping = false;
  lastBpToggleUs = micros();
  markUserActivity();
  buttonLockoutUntilMs = millis() + BUTTON_LOCKOUT_MS;
  ts3WasPressed = (VPORTA.IN & PIN_TS3_bm) == 0;
  ts4WasPressed = (VPORTA.IN & PIN_TS4_bm) == 0;
  ts3HoldTriggered = true;
  ts4HoldTriggered = true;
  chordWasPressed = ts3WasPressed && ts4WasPressed;
  chordHoldTriggered = chordWasPressed;
  showIdleMarker();
}

void markUserActivity() {
  lastUserActivityMs = millis();
}

void setMax232Power(bool on) {
  if (on) {
    VPORTA.OUT &= ~PIN_MAX_ACTIVE_bm;
  } else {
    VPORTA.OUT |= PIN_MAX_ACTIVE_bm;
  }
}

void sendBurst() {
  uint8_t idleOut = VPORTA.OUT & ~(PIN_TX_T1_bm | PIN_TX_T2_bm);
  uint8_t t1Out = idleOut | PIN_TX_T1_bm;
  uint8_t t2Out = idleOut | PIN_TX_T2_bm;

#if F_CPU == 1000000L
  uint8_t count = TX_CYCLES;
  __asm__ __volatile__(
    "1:" "\n\t"
    "out %[port], %[t1]" "\n\t"
    "nop" "\n\t"
    "nop" "\n\t"
    "nop" "\n\t"
    "nop" "\n\t"
    "nop" "\n\t"
    "nop" "\n\t"
    "nop" "\n\t"
    "nop" "\n\t"
    "nop" "\n\t"
    "nop" "\n\t"
    "nop" "\n\t"
    "out %[port], %[t2]" "\n\t"
    "nop" "\n\t"
    "nop" "\n\t"
    "nop" "\n\t"
    "nop" "\n\t"
    "nop" "\n\t"
    "nop" "\n\t"
    "nop" "\n\t"
    "nop" "\n\t"
    "nop" "\n\t"
    "dec %[count]" "\n\t"
    "brne 1b" "\n\t"
    "out %[port], %[idle]" "\n\t"
    : [count] "+r" (count)
    : [port] "I" (_SFR_IO_ADDR(VPORTA.OUT)),
      [t1] "r" (t1Out),
      [t2] "r" (t2Out),
      [idle] "r" (idleOut)
  );
#else
  for (uint8_t i = 0; i < TX_CYCLES; i++) {
    VPORTA.OUT = t1Out;
    delayMicroseconds(TX_HALF_PERIOD_US);
    VPORTA.OUT = t2Out;
    delayMicroseconds(TX_HALF_PERIOD_US);
  }

  VPORTA.OUT = idleOut;
#endif
}

uint16_t readEchoAdc() {
  if (!adcPowered) {
    setAdcPower(true);
  }

  while (ADC0.COMMAND & ADC_START_gm) {
  }
  ADC0.MUXPOS = ECHO_ADC_MUXPOS;
  ADC0.INTFLAGS = ADC_RESRDY_bm;
  ADC0.COMMAND = ECHO_ADC_COMMAND;
  while (!(ADC0.INTFLAGS & ADC_RESRDY_bm)) {
  }

#if ECHO_ADC_RESULT_SCALE == 2
  return (uint16_t)(ADC0.RESULT << 2);
#elif ECHO_ADC_RESULT_SCALE == 0
  return (uint16_t)ADC0.RESULT;
#else
  return (uint16_t)(ADC0.RESULT >> 2);
#endif
}
