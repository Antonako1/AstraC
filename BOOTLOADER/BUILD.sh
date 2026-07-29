#!/bin/sh
# Build multi-stage bootloader + kernel floppy image
# Requires: AstraC in PATH or at ../build/Release/AstraC

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR" || exit 1

if command -v AstraC >/dev/null 2>&1; then
    ASTRAC="AstraC"
elif [ -f "../build/Release/AstraC" ]; then
    ASTRAC="../build/Release/AstraC"
elif [ -f "$HOME/.local/bin/AstraC" ]; then
    ASTRAC="$HOME/.local/bin/AstraC"
else
    echo "[ERROR] AstraC not found. Set ASTRAC env var or place it in PATH."
    exit 1
fi

echo "=== Assembling BOOTLOADER ==="
"$ASTRAC" asm BOOTLOADER.AS bits 16 org 7C00 || { echo "BOOTLOADER failed"; exit 1; }

echo "=== Assembling SECOND_STAGE ==="
"$ASTRAC" asm SECOND_STAGE.AS bits 16 org 7E00 || { echo "SECOND_STAGE failed"; exit 1; }

echo "=== Compiling KERNEL ==="
"$ASTRAC" comp KERNEL.AC bits 32 org 10000 verbose warn err || { echo "KERNEL failed"; exit 1; }

echo "=== Creating floppy image ==="
# 1.44 MB = 1474560 bytes = 2880 sectors of 512 bytes
dd if=/dev/zero of=floppy.img bs=512 count=2880 2>/dev/null
dd if=BOOTLOADER.BIN   of=floppy.img bs=512 seek=0 count=1 conv=notrunc 2>/dev/null
dd if=SECOND_STAGE.BIN of=floppy.img bs=512 seek=1 count=8 conv=notrunc 2>/dev/null
dd if=KERNEL.BIN       of=floppy.img bs=512 seek=9 count=9 conv=notrunc 2>/dev/null

echo "floppy.img created (1474560 bytes)"

# Show sizes
STAT_CMD="stat -c%s"
[ "$(uname)" = "Darwin" ] && STAT_CMD="stat -f%z"
echo "BOOTLOADER.BIN:     $($STAT_CMD BOOTLOADER.BIN) bytes"
echo "SECOND_STAGE.BIN:   $($STAT_CMD SECOND_STAGE.BIN) bytes"
echo "KERNEL.BIN:         $($STAT_CMD KERNEL.BIN) bytes"
echo ""
echo "QEMU command:"
echo "  qemu-system-i386 -drive format=raw,file=$SCRIPT_DIR/floppy.img,if=floppy"
