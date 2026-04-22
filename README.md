# Cheep4

Yet another Eurorack digital oscillator

![](./doc/Cheep4_front.jpg)

## Abstract

Cheep4 is a continuation of a series of low-spec / low-cost digital oscillator modules for Eurorack whose primary purpose is to demonstrate using extreme low-end microcontrollers to do DSP. The series includes the following earlier modules:

[cheep_mod](https://github.com/emeb/Old_Website/tree/main/synth/cheep_mod) - the original stripped-down done with an STM32F030 and a SPI DAC.

[cheep2](https://github.com/emeb/Old_Website/tree/main/synth/cheep2) - 2nd generation uses an STM32F031 and a stereo I2S DAC.

[cheep3a](https://github.com/emeb/cheep3a) - 3rd generation based on a CH32V003 and stereo I2S DAC faked out with a SPI interface.

Cheep4 dials up the functionality with a more advanced MCU - the STM32C542CCT6 which although more powerful is still inexpensive (under $2) - and a decent quality I2S DAC, as well as a more complete set of controls for the 1V/Octave input (Coarse, Fine, FM pots). Similar to the cheep3a, it has a button & LED mode select interface which provides seven distinct algorithms and nonvolatile memory to save the mode across power cycles. Behind the panel there are also USB and Qwiic connectors for upgrades and expansion.

## Features

- STM32C542CCT6 MCU with
  
  - Arm Cortex M33 at 144MHz
  
  - 256kB Flash
  
  - 64kB SRAM
  
  - CORDIC math coprocessor

- PCM5100A I2S DAC

- GD25Q32 4MB SPI flash memory

- USB port

- Qwiic I2C port

- 16-pin shrouded Eurorack power connector with +/-12V only

- 6x 9mm potentiometers
  
  - Coarse pitch
  
  - Fine pitch
  
  - FM pitch attenuator
  
  - CV1, CV2, CV3 offsets

- 8x 3.5mm mono jacks
  
  - 1V/Oct pitch input (+/-5V range)
  
  - FM pitch input
  
  - CV1, CV2, CV3 inputs (+/-5V range)
  
  - Sync input (digital, 0.7V threshold)
  
  - 2 +/-5V outputs

- Button + RGB LED for mode selection.

## Firmware

Firmware included with this project includes test code for all the critical MCU features as well as functional oscillator applications. All code is built with my own simplified make-based system and needs only an Arm GCC compiler and an ST-Link programmer. Code is based on the ST HAL2 hardware interface library as well as the Arm CMSIS low-level drivers and register definitions, both of which are included in this repository.

### Prerequisites

- Arm GCC compiler

- ST-Link programmer

- STM32 Cube Programmer application installed (needed for interfacing to ST-Link programming hardware)

### Building

Change directories into the desired subdirectory and run `make` - this may require editing the Makefile to point to the installation locations of you prerequisites.

## Hardware

Kicad 9.0.x design files are included for both the passive front panel and active back panel. A Libreoffice ODS spreadsheet of the Bill of Materials (BOM) is available, or you can generate your own desired format from Kicad. No special assembly techniques are required beyond basic SMT and thru-hole soldering. A [schematic](./doc/cheep4_sch.pdf) is available.

## Lessons Learned

This project involved a fair bit of new material for me - not only is the STM32C542 a new family of MCU, but ST has chosen to migrate to a new HAL2 API for interfacing to their hardware, as well as a newe CubeMX2 configuration & code generation application. What follows are some notes on the new hardware, firmware and software experiences.



TBD...
