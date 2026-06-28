NearSens v11 ready, firmware for Arduino IDE / megaTinyCore / ATtiny1626

Arduino IDE settings:
- Board: ATtiny3226/1626/826/426
- Chip: ATtiny1626
- Clock: 1 MHz internal
- Variant: no Optiboot / no bootloader
- Programmer: jtag2updi
- Upload: Upload Using Programmer

Use:
- After startup, the LCD shows 8888. This is intentional as a test of all segments.
- TS3 short press: perform one measurement.
- TS4 short press: toggle front/body measurement reference. Body mode adds the device length offset.
- TS3 + TS4 together for 5 s: calibration at 500 mm. Place a flat object exactly 50 cm from the measurement reference and hold both buttons.

LCD codes:
- 8888: idle / LCD test.
- 0500 after calibration: 500 mm calibration stored.
- 4001: no echo.
- 4002: echo too early / stuck / crosstalk.
- 4003: result outside the usable window.
- 4004: calibration failed.
- 4005: RX / ADC path error.
- 4006: ringdown too long.

Main v11 changes:
- Removed 100/300/500 multi-point correction as the default model because it could use points incorrectly and flatten the range above 30 cm.
- Calibration now works as a safe offset from one 500 mm point: result = raw + (500 - raw500).
- Old EEPROM v5/v4/v3 data is not used. After upload, perform a new 500 mm calibration.
- A single long press of TS3 or TS4 no longer stores 100/300 mm points.
- Error codes use the 40xx format instead of 9999.
- Idle 8888 remains intentional, without automatic deep sleep blanking.
