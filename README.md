# MakeyDooey — Firmware Repository

Firmware for the [MakeyDooey](https://makeydooey.github.io/Software) system.
(_new website!_)

## Overview
(latest add: feature-3-update)

This repository contains the firmware that runs on the MakeyDooey embedded platform. The project targets a heterogeneous dual-core MCU setup (Cortex‑M7 and Cortex‑M4 cores) on the **STM32H755** and provides the low-level software, board bring‑up, and runtime environment required by the MakeyDooey hardware.

The project is split into two core domains that operate asynchronously but coordinate via shared memory (SRAM4 mapped at `0x38000000`):

### Cortex-M7 (Manager Core)
- Located in the `/CM7/` directory.
- Runs a **FreeRTOS**-based application.
- Configures the system clocks, Memory Protection Unit (MPU), and manages the dual-core boot synchronization via Hardware Semaphores (HSEM).
- Hosts a serial Command Line Interface (CLI) over UART3. This allows users to dynamically enqueue operations ("steps" with opcodes and payloads) into a shared mailbox.
- CLI commands include `peek` (view M4 status), `add` (add a step for M4 to execute), `run` (mode 0=Stop, 1=Once, 2=Loop), and `clear` (wipe mailbox).

### Cortex-M4 (Worker Core)
- Located in the `/CM4/` directory.
- Runs a **bare-metal** image.
- Executes low-level hardware interactions based on commands stored in the Shared RAM mailbox by the M7.
- Operates a `while(1)` superloop using `__WFI()` for power efficiency, polling the user Blue Button (PC13) and maintaining a heartbeat counter to indicate to the M7 that it remains responsive.
- Functions include complex LED/GPIO control sequences (with individual RGB toggling) and setting hardware PWM on Timer 1 (`htim1`).

### Shared Memory Protocol & Instruction CLI Examples
The two cores communicate through a structured `SharedMemory_t` protocol defined in `/Common/Inc/shared_logic.h`. The M7 core writes up to 64 `Step_t` instructions (consisting of custom opcodes and bitmasked pin values) and triggers execution by setting a `run_flag`. The M4 core continuously parses this mailbox and takes action accordingly.

You can queue execution steps using the M7 UART CLI by utilizing the `add <opcode> <payload_value>` command. The Worker (M4) currently processes two main `opcode` instructions:

#### Opcode 1: LED & GPIO Control
For LED control, the `payload_value` uses a bitmask to designate target, action, and type. You must combine (sum) the properties you want:
- **Base Type Flag**: `FLAG_GPIO_TYPE` (1) + `FLAG_LED_TYPE` (2) = **3**
- **Action Flags (choose one)**: `FLAG_ACTION_ON` (4), `FLAG_ACTION_OFF` (8), `FLAG_ACTION_TOG` (16)
- **Target LED Flags (can be combined)**: `MASK_RED` (32), `MASK_YELLOW` (64), `MASK_GREEN` (128)

**CLI Examples for Opcode 1**:
1. **Turn ON Red LED**: Base(3) + ON(4) + Red(32) = 39.
   => `add 1 39`
2. **Turn OFF Red LED**: Base(3) + OFF(8) + Red(32) = 43.
   => `add 1 43`
3. **Turn ON Green LED**: Base(3) + ON(4) + Green(128) = 135.
   => `add 1 135`
4. **Toggle Yellow LED**: Base(3) + TOGGLE(16) + Yellow(64) = 83.
   => `add 1 83`
5. **Turn ON Red AND Yellow**: Base(3) + ON(4) + Red(32) + Yellow(64) = 103.
   => `add 1 103`

#### Opcode 2: PWM Control
For PWM control on Timer 1, the `payload_value` directly corresponds to the raw Capture/Compare Register (CCR) value.
**CLI Examples for Opcode 2**:
1. **Set Duty Cycle to 500**:
   => `add 2 500`

#### Running a Command Sequence
Once you have added one or more steps to the mailbox, start execution using the `run` command mode.
- `run 1`: Run the sequence once.
- `run 2`: Run the sequence in continuous loop.
- `run 0`: Stop execution and idle.

## Progress

[feature-2-sprint](https://github.com/MakeyDooey/Firmware-NUCLEO-H755ZI-Q/tree/feature-2-sprint)

[feature-1-sprint](https://github.com/MakeyDooey/Firmware-NUCLEO-H755ZI-Q/tree/feature-1-sprint)

## Getting started

1. Install toolchain and dependencies (see [dependencies.md](https://github.com/MakeyDooey/Firmware/blob/main/dependencies.md)). The project enforces strict dependency alignment between Local and CI/CD environments via helper scripts.
2. Build the Project:
   - Use the Docker environment to build the firmware to ensure consistent tool versions: `make docker-build`.
   - Or alternatively use the local provided wrapper script `./makey.sh` which runs a Docker container (`h7-builder`) for fully isolated builds leveraging `bear` for compile commands.
   - Calling `make all` orchestrates compiling the M7 sub-project followed sequentially by the M4 sub-project.
3. Flash with OpenOCD: `make flash` (this programs both the CM7 and CM4 `.elf` files).
    - see [Makefile](https://github.com/MakeyDooey/Firmware/blob/main/Makefile) for more information on how Docker + OpenOCD are configured for this hardware. 
4. Boot sequence details:
   - **M7 Boot**: Initializes MPU for SRAM4 (non-cacheable), sets up system clocks, wipes the shared RAM space to zero, and triggers an HSEM sync event to release the M4 core from sleep.
   - **M4 Boot**: Inherits clock settings from the D2 domain, synchronizes via the HSEM flag, spins up Timer 1 and executes tasks from the shared mailbox.
