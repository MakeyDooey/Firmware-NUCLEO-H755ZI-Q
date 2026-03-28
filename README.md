# MakeyDooey — Firmware Repository

Firmware for the [MakeyDooey](https://makeydooey.github.io/Software) system.
(_new website!_)

## Overview
(latest add: feature-2-update)

This repository contains the firmware that runs on the MakeyDooey embedded platform. The project targets a heterogeneous MCU setup (Cortex‑M7 and Cortex‑M4 cores) and provides the low-level software, board bring‑up, and runtime environment required by the MakeyDooey hardware.

## Progress

[feature-2-sprint](https://github.com/MakeyDooey/Firmware/tree/feature-2-sprint)

[feature-1-sprint](https://github.com/MakeyDooey/Firmware/tree/feature-1-sprint)

## Getting started
1. Install toolchain and dependencies (see [dependencies.md](https://github.com/MakeyDooey/Firmware/blob/main/dependencies.md)).
2. Use the Docker environment to build and run the firmware to ensure consistent tool versions: ```make docker-build```
3. Flash with OpenOCD: ```make flash```
    - see [Makefile](https://github.com/MakeyDooey/Firmware/blob/main/Makefile) for more information on how Docker + OpenOCD are configure for this hardware 
4. Boot sequence:
   - M7: FreeRTOS-based firmware image
   - M4: Bare-metal image (with custom UART driver)
