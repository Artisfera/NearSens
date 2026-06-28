#ifndef NEARSENS_H
#define NEARSENS_H

#include <Arduino.h>
#include <avr/io.h>
#include <avr/sleep.h>

#define PIN_TX_T2_bm      PIN4_bm
#define PIN_TX_T1_bm      PIN5_bm
#define PIN_ECHO_bm       PIN6_bm
#define PIN_MAX_ACTIVE_bm PIN7_bm
#define PIN_TS4_bm        PIN1_bm
#define PIN_TS3_bm        PIN2_bm

#define PIN_LCD_BP_bm     PIN0_bm
#define PIN_LCD_LD1_bm    PIN1_bm
#define PIN_LCD_LD2_bm    PIN2_bm
#define PIN_LCD_LD3_bm    PIN3_bm
#define PIN_LCD_LD4_bm    PIN4_bm
#define PIN_LCD_BL_bm     PIN5_bm

#define PIN_LCD_A_bm      PIN0_bm
#define PIN_LCD_B_bm      PIN1_bm
#define PIN_LCD_C_bm      PIN2_bm
#define PIN_LCD_D_bm      PIN3_bm
#define LCD_BCD_MASK      (PIN_LCD_A_bm | PIN_LCD_B_bm | PIN_LCD_C_bm | PIN_LCD_D_bm)

#define ECHO_ADC_MUXPOS   ADC_MUXPOS_AIN6_gc

#define RUN_MEASURE       0
#define RUN_CALIBRATE     1

#define REFERENCE_FRONT   0
#define REFERENCE_BODY    1

#if defined(ADC_MODE_SINGLE_8BIT_gc)
#define ECHO_ADC_COMMAND       (ADC_MODE_SINGLE_8BIT_gc | ADC_START_IMMEDIATE_gc)
#define ECHO_ADC_RESULT_SCALE  2
#elif defined(ADC_MODE_SINGLE_10BIT_gc)
#define ECHO_ADC_COMMAND       (ADC_MODE_SINGLE_10BIT_gc | ADC_START_IMMEDIATE_gc)
#define ECHO_ADC_RESULT_SCALE  0
#else
#define ECHO_ADC_COMMAND       (ADC_MODE_SINGLE_12BIT_gc | ADC_START_IMMEDIATE_gc)
#define ECHO_ADC_RESULT_SCALE  255
#endif

#define EEPROM_MAGIC0_ADDR   0
#define EEPROM_MAGIC1_ADDR   1
#define EEPROM_VERSION_ADDR  2
#define EEPROM_RAW100_L_ADDR 3
#define EEPROM_RAW100_H_ADDR 4
#define EEPROM_RAW300_L_ADDR 5
#define EEPROM_RAW300_H_ADDR 6
#define EEPROM_RAW500_L_ADDR 7
#define EEPROM_RAW500_H_ADDR 8
#define EEPROM_FLAGS_ADDR    9
#define EEPROM_CHECKSUM_ADDR 10

static const uint8_t EEPROM_MAGIC0 = 0x4E;
static const uint8_t EEPROM_MAGIC1 = 0x53;
static const uint8_t EEPROM_VERSION = 13;
static const uint16_t FIRMWARE_BUILD_ID = 27;
static const uint8_t EEPROM_FLAG_100_VALID = 0x01;
static const uint8_t EEPROM_FLAG_300_VALID = 0x02;
static const uint8_t EEPROM_FLAG_500_VALID = 0x04;
static const uint8_t EEPROM_FLAGS_ALL_VALID = EEPROM_FLAG_100_VALID |
                                              EEPROM_FLAG_300_VALID |
                                              EEPROM_FLAG_500_VALID;

extern const uint8_t LCD_LD_MASKS[4];

static const uint32_t LCD_BP_TOGGLE_US = 5000UL;
static const uint16_t LCD_LATCH_PULSE_US = 3;

static const uint8_t TX_CYCLES = 14;
static const uint8_t TX_HALF_PERIOD_US = 12;

static const uint8_t MEASURE_PINGS = 25;
static const uint8_t MEASURE_MIN_VALID = 3;
static const uint8_t MEASURE_MIN_CLUSTER = 3;
static const uint16_t MEASURE_CLUSTER_MM = 14;

static const uint8_t CAL_PINGS = 25;
static const uint8_t CAL_MIN_VALID = 3;
static const uint8_t CAL_MIN_CLUSTER = 3;
static const uint16_t CAL_CLUSTER_MM = 18;

static const uint16_t INTER_PING_MS = 17;
static const uint16_t BUTTON_LOCKOUT_MS = 300;
static const uint16_t CAL_HOLD_MS = 5000;
static const uint16_t TS3_CLICK_WINDOW_MS = 1100;
static const uint8_t TS3_RAW_CLICK_COUNT = 5;
static const uint16_t BUTTON_MAX_SHORT_PRESS_MS = 450;
static const uint16_t MEASURE_POWER_SETTLE_MS = 320;
static const uint8_t ADC_WARMUP_READS = 32;
static const uint16_t STARTUP_DISPLAY_MS = 900;
static const uint16_t REFERENCE_BANNER_MS = 2000;
static const uint32_t DEEP_SLEEP_TIMEOUT_MS = 60000UL;

static const uint16_t BASELINE_SAMPLES = 22;
static const uint16_t ECHO_GUARD_AFTER_TX_US = 145;
static const uint32_t ECHO_TIMEOUT_FROM_TX_START_US = 6200UL;
static const uint16_t ECHO_MIN_MM = 60;
static const uint16_t ECHO_MAX_MM = 950;
static const bool ENABLE_FAR_FALLBACK = false;

static const uint16_t ADC_THRESHOLD_MIN = 5;
static const uint16_t ADC_THRESHOLD_EXTRA = 3;
static const uint16_t ADC_FAR_THRESHOLD_MIN = 4;
static const uint16_t ADC_FAR_THRESHOLD_EXTRA = 1;
static const uint16_t ADC_CONFIRM_MARGIN = 0;
static const uint16_t ADC_STRONG_CONFIRM_MARGIN = 5;
static const uint16_t ADC_CONFIRM_WINDOW_US = 170;
static const uint16_t ADC_NOISE_FAULT = 120;

static const uint8_t ECHO_CANDIDATE_MAX = 8;
static const uint8_t ECHO_MIN_PULSE_HITS = 2;
static const uint16_t ECHO_MIN_PULSE_WIDTH_US = 12;
static const uint16_t ECHO_MAX_PULSE_WIDTH_US = 700;
static const uint16_t ECHO_PEAK_TIE_MARGIN = 2;
static const uint16_t ADC_RAIL_LOW = 25;
static const uint16_t ADC_RAIL_HIGH = 998;

static const uint16_t CAL_100_TARGET_MM = 100;
static const uint16_t CAL_300_TARGET_MM = 300;
static const uint16_t CAL_500_TARGET_MM = 500;
static const uint8_t CAL_POINT_SAMPLES = 9;
static const uint8_t CAL_POINT_MIN_GOOD = 3;
static const uint16_t CAL_POINT_MAX_SPREAD_MM = 25;
static const uint16_t CAL_RAW_MIN_SEPARATION_100_300_MM = 45;
static const uint16_t CAL_RAW_MIN_SEPARATION_300_500_MM = 45;
static const uint16_t CAL_VALID_RAW100_MIN_MM = 35;
static const uint16_t CAL_VALID_RAW100_MAX_MM = 330;
static const uint16_t CAL_VALID_RAW300_MIN_MM = 90;
static const uint16_t CAL_VALID_RAW300_MAX_MM = 650;
static const uint16_t CAL_VALID_RAW500_MIN_MM = 180;
static const uint16_t CAL_VALID_RAW500_MAX_MM = 1000;
static const uint16_t FAR_ECHO_START_MM = 280;
static const uint16_t DEVICE_REFERENCE_OFFSET_MM = 75;
static const uint16_t LCD_DISPLAY_MAX = 9998;

static const uint8_t DISPLAY_IDLE = 0;
static const uint8_t DISPLAY_VALUE = 1;
static const uint8_t DISPLAY_BANNER = 2;
static const uint8_t LCD_RAW_BLANK = 0x0F;

static const uint16_t LCD_NO_RESULT = 4000;
static const uint16_t LCD_NO_ECHO = 4001;
static const uint16_t LCD_ECHO_STUCK = 4002;
static const uint16_t LCD_OUT_OF_RANGE = 4003;
static const uint16_t LCD_CAL_FAIL = 4004;
static const uint16_t LCD_CAL_DATA_INVALID = 4005;
static const uint16_t LCD_CAL_SPREAD = 4006;
static const uint16_t LCD_RX_FAULT = 4007;
static const uint16_t LCD_RAW_ON = 7001;
static const uint16_t LCD_RAW_OFF = 7000;
static const uint16_t LCD_REF_FRONT = 2001;
static const uint16_t LCD_REF_BODY = 2002;
static const uint16_t LCD_CAL_SUCCESS = 1111;

extern bool bpLevel;
extern bool measuringNow;
extern bool ts3WasPressed;
extern bool ts4WasPressed;
extern bool ts3HoldTriggered;
extern bool ts4HoldTriggered;
extern bool chordWasPressed;
extern bool chordHoldTriggered;
extern bool calibrationValid;
extern bool calibrationWizardActive;
extern bool lastMeasurementValid;
extern bool adcPowered;
extern bool deepSleeping;
extern bool rawMode;

extern uint32_t lastBpToggleUs;
extern uint32_t startupUntilMs;
extern uint32_t buttonLockoutUntilMs;
extern uint32_t ts3HoldStartMs;
extern uint32_t ts4HoldStartMs;
extern uint32_t chordHoldStartMs;
extern uint32_t animationLastMs;
extern uint32_t bannerUntilMs;
extern uint32_t lastUserActivityMs;
extern uint32_t ts3ClickFirstMs;

extern uint16_t displayValue;
extern uint16_t physicallyDisplayedValue;
extern uint8_t animationIndex;
extern uint8_t displayMode;
extern uint8_t referenceMode;
extern uint8_t calibrationWizardStep;
extern uint8_t calibrationFlags;
extern uint8_t ts3ClickCount;

extern uint16_t calibrationRaw100Mm;
extern uint16_t calibrationRaw300Mm;
extern uint16_t calibrationRaw500Mm;
extern uint16_t pendingCalibrationRaw100Mm;
extern uint16_t pendingCalibrationRaw300Mm;
extern uint16_t pendingCalibrationRaw500Mm;
extern uint16_t lastRawMeasurementMm;

void configurePins();
void configureAdc();
void idleUntilInterrupt();
void serviceDeepSleep();
void enterDeepSleep();
void wakeFromDeepSleep();
void markUserActivity();
void serviceBackplane();
void serviceBackplaneAt(uint32_t nowUs);
void serviceButtons();
void runUserMeasurement();
void toggleReferenceMode();
void toggleRawMode();
void serviceTs3ClickAction(uint32_t nowMs);
void registerTs3ShortClick(uint32_t nowMs);
void showMeasurementResult(uint16_t rawMm);
void serviceDisplay();
void startMeasureAnimation();
void stopMeasureAnimation();
void serviceMeasureAnimation();
void setDisplayValue(uint16_t value);
void showIdleMarker();
void showReferenceBanner();
void showRawModeBanner();
void lcdSetBlank(bool blank);
void setMax232Power(bool on);
void setAdcPower(bool on);
void prepareMeasurementPower();
void finishMeasurementPower();
void sendBurst();
uint16_t readEchoAdc();
uint16_t measurePacket(uint8_t runType, bool *ok);
uint16_t measureOnePing(bool *valid);
uint8_t pingSpacingMs(uint8_t pingIndex);
uint16_t captureBaseline(uint16_t *minAdc, uint16_t *maxAdc);
uint16_t chooseClusterResult(uint16_t *values, uint8_t count, uint16_t clusterMm, uint8_t minCluster, bool *stable);
void sortValues(uint16_t *values, uint8_t count);
bool applyCalibration(uint16_t rawMm, uint16_t *correctedMm);
bool interpolateCalibratedValue(int32_t rawMm, int32_t rawA, int32_t targetA, int32_t rawB, int32_t targetB, uint16_t *correctedMm);
bool calibrationFullSetValid(uint16_t raw100Mm, uint16_t raw300Mm, uint16_t raw500Mm);
bool calibrationPairValid(uint16_t rawA, uint16_t rawB, uint16_t minSep);
bool calibrationPointValid(uint8_t flag, uint16_t rawMm);
bool calibrationRawInWindow(uint16_t rawMm, uint16_t minMm, uint16_t maxMm);
void startCalibrationWizard();
void cancelCalibrationWizard();
void showCalibrationWizardPrompt();
void nextCalibrationWizardPoint();
void handleCalibrationWizardConfirm();
bool captureCalibrationPoint(uint16_t targetMm, uint16_t *rawMm, uint16_t *errorCode);
void loadCalibration();
bool loadCalibrationV10();
void saveCalibration();
uint8_t calibrationChecksumV10(uint16_t raw100Mm, uint16_t raw300Mm, uint16_t raw500Mm, uint8_t flags);
uint16_t timeOfFlightToMm(uint32_t tofUs);
uint16_t absDiffAdc(uint16_t a, uint16_t b);
uint16_t absDiff16(uint16_t a, uint16_t b);
bool dueMs(uint32_t nowValue, uint32_t dueValue);
bool dueUs(uint32_t nowValue, uint32_t dueValue);
void lcdShowValue(uint16_t value);
void lcdShowRawBcd4(uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3);
void lcdWriteDigit(uint8_t digitIndex, uint8_t bcd);
void lcdSetBcd(uint8_t bcd);
void lcdPulseLatch(uint8_t digitIndex);

#endif
