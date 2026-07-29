#!/bin/sh
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
"$SCRIPT_DIR/MAKE.sh" "$@" && "$SCRIPT_DIR/RUN.sh" "$@"
