# NearSens

NearSens is a small contactless distance meter for short range measurement. It uses a 40 kHz ultrasonic transmitter and receiver, an ATtiny1626 microcontroller, an analog receiver path, and a four-digit LCD.

The current prototype is built around a practical 5 V setup and a useful range of about 100 to 500 mm. The goal was not to make a universal sensor module. The goal was to make a simple device that can be built, tested, calibrated, and understood without hiding the hardware behind a black box.

## Main assumptions

- external 5 V DC supply
- ultrasonic time-of-flight measurement
- practical range around 100 to 500 mm
- four-digit LCD readout
- simple two-button interface
- five-point calibration stored in EEPROM
- low part count and low cost where possible
- firmware kept readable enough to debug on real hardware

## How it works

The microcontroller sends a short ultrasonic burst through the transmitter driver. The receiver picks up the echo, the analog path amplifies and filters it, and the firmware searches for a valid reflected signal in the expected time window.

The distance is calculated from echo return time. Calibration points at 100, 200, 300, 400 and 500 mm are stored in EEPROM and used to correct later measurements.

## Hardware overview

The main hardware blocks are:

- ATtiny1626 microcontroller
- ST232CDR used as the ultrasonic transmitter driver
- 40 kHz ultrasonic transmitter and receiver
- LM324 receiver path
- MC14543BDG LCD drivers
- DE117 four-digit LCD
- two tactile buttons, TS3 and TS4
- external 5 V input through PS1

R4 is a 220 ohm THT resistor added in the receiver/filter path after testing. It is listed separately in the BOM.

## Repository layout

```text
docs/
  NearSens_project_documentation.pdf
  NearSens_user_guide.pdf

hardware/
  schematic/   schematic preview and EasyEDA source
  pcb/         PCB source, outline and previews
  gerber/      PCB production files
  enclosure/   STL models

firmware/
  NearSens_1.0.0/   current firmware source

bom/
  NearSens_BOM_v1.0.0.csv

revisions/
  earlier schematic, PCB, firmware and enclosure versions

JOURNAL.md
  build log exported from Stasis
```

## Documentation

Start here:

- [Project documentation](docs/NearSens_project_documentation.pdf)
- [Use and calibration guide](docs/NearSens_user_guide.pdf)
- [Bill of materials](bom/NearSens_BOM_v1.0.0.csv)

The `revisions` folder is an archive of earlier work. The current files are in `docs`, `hardware`, `firmware` and `bom`.

## Firmware

Current firmware is in:

```text
firmware/NearSens_1.0.0/
```

It was prepared for Arduino IDE with the megaTinyCore package and ATtiny1626 running at 5 MHz.

Main controls:

- TS3 makes a measurement
- TS4 toggles the 75 mm reference correction
- holding TS3 and TS4 enters calibration
- five quick TS3 clicks toggle RAW mode

Normal use does not need a computer. A 5 V DC supply is enough to power the prototype and take measurements.

## Calibration

Calibration uses five points:

```text
100 mm
200 mm
300 mm
400 mm
500 mm
```

Use a flat, hard target placed perpendicular to the ultrasonic transducers. Soft materials, angled surfaces and nearby reflections can make the result unstable.

## Status

This repository contains the v1.0.0 prototype documentation, firmware and hardware files. It is a working prototype, not a finished commercial product. Some parts of the history are kept in `revisions` because they show how the design changed over time.


## License

NearSens is source-available for non-commercial community use.

Firmware and source code are licensed under the PolyForm Noncommercial License 1.0.0. The official license text is in [LICENSE](LICENSE) and [LICENSES/PolyForm-Noncommercial-1.0.0.txt](LICENSES/PolyForm-Noncommercial-1.0.0.txt).

Hardware design files, PCB files, CAD files, Gerbers, documentation, BOM, images and journal files are licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International. The official license text is in [LICENSES/CC-BY-NC-SA-4.0.txt](LICENSES/CC-BY-NC-SA-4.0.txt).

Required attribution: `NearSens by Patryk Ankudowicz (Artisfera)`.

Commercial use is not granted by these licenses. Contact the author for separate permission.

## Author

<<<<<<< Updated upstream
Patryk Ankudowicz (Artisfera)
=======
Patryk Ankudowicz


![37985225-6b87-468d-819c-fa1fb3e9c759](https://stasis.hackclub-assets.com/images/1782644021175-dzq1qj.jpg)

![0deb3eeb-d8f6-42e3-ba4b-9c512cce84f7](https://stasis.hackclub-assets.com/images/1782644021235-uzoq6a.jpg)

![9b99e4cb-7a58-41e2-9bc0-3cf050692205](https://stasis.hackclub-assets.com/images/1782644021241-bn8yt4.jpg)
>>>>>>> Stashed changes
