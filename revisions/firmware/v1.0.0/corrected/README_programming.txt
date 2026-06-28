NearSens 1.0.0 - microcontroller firmware

This directory contains NearSens firmware source files for the ATtiny1626 microcontroller. The code is prepared as an Arduino IDE sketch using megaTinyCore.

Environment
Arduino IDE with the megaTinyCore package
Microcontroller: ATtiny1626
Clock is selected in the build environment

Main sketch file
NearSens_1.0.0.ino

File split
NearSens.h - pin definitions, constants, states and function declarations
Buttons.cpp - TS3, TS4, RAW mode, 75 mm reference correction and calibration entry handling
Calibration.cpp - 100/200/300/400/500 mm calibration and EEPROM storage
Display.cpp - LCD handling and display codes
Hardware.cpp - pin setup, ADC setup, TX path power control and sleep handling
Measurement.cpp - ultrasonic measurement and echo qualification
State.cpp - firmware state variables
Utils.cpp - helper functions

Main features
Distance measurement
Calibration point storage in EEPROM
RAW mode
LCD error codes
75 mm reference correction
Reduced power use by disabling unused blocks outside measurement

Notes
This package does not include .hex, .bin, .elf, .eep or .map files. They were not included as build artifacts.
Do not generate a new binary without confirming the exact Arduino IDE/megaTinyCore build configuration used for this prototype.
