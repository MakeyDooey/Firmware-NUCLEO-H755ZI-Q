#!/bin/bash
docker run --rm -v $(pwd):/workspace -w /workspace h7-builder bear -- make -j$(nproc)
openocd -f openocd.cfg -c "program CM7_f.elf verify; program CM4_f.elf verify; reset run; exit"
