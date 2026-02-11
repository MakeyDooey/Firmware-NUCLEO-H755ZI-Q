#!/bin/bash
docker run --rm -v $(pwd):/workspace -w /workspace h7-builder bear -- make -j$(nproc)
openocd -f openocd.cfg -c "program ./CM7/build/Firmware_CM7.elf verify; program ./CM4/build/Firmware_CM4.elf verify; reset run; exit"
