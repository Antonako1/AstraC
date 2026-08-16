#!/bin/bash
qemu-system-i386 -drive format=raw,file="$PWD/TESTS/OUTPUT/floppy.img",if=floppy \
-vga std -boot order=a -m 64M