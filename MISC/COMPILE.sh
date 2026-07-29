#!/bin/sh
# USAGE: COMPILE.sh <source_file> <output_file>

if [ $# -lt 2 ]; then
    echo "USAGE: $0 <source_file> <output_file>"
    exit 1
fi

CC="${CC:-gcc}"
CFLAGS="-Wall -Wextra -O2 -D_DEFAULT_SOURCE -D_CRT_SECURE_NO_WARNINGS"

if ! command -v "$CC" >/dev/null 2>&1; then
    echo "[ERROR] Compiler '$CC' not found."
    exit 1
fi

"$CC" $CFLAGS -o "$2" "$1"
if [ $? -ne 0 ]; then
    echo "[ERROR] Compilation failed."
    exit 1
fi
