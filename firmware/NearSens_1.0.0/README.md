NearSens 1.0.0 - microcontroller firmware

Environment
Arduino IDE with the megaTinyCore package
Microcontroller: ATtiny1626
Prototype v1.0.0 clock: 5 MHz

Main file
NearSens_1.0.0.ino

File split
NearSens.h - pin definitions, constants, states and function declarations
Buttons.cpp - TS3, TS4, RAW mode, 75 mm correction and calibration entry handling
Calibration.cpp - 100/200/300/400/500 mm calibration and EEPROM storage
Display.cpp - LCD handling and display codes
Hardware.cpp - pin setup, ADC setup, TX path power control and sleep handling
Measurement.cpp - ultrasonic measurement and echo qualification
State.cpp - firmware state variables
Utils.cpp - helper functions

Main features
Measurement after pressing TS3
75 mm correction toggled with TS4
RAW mode after five quick TS3 clicks
Calibration after holding TS3 and TS4
Calibration point storage in EEPROM
LCD codes 8888, 1111, 4000-4007, 7000 and 7001
