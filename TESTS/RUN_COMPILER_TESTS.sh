#!/usr/bin/env bash
# AstraC Compiler Feature Test Runner for Linux/macOS
# Tests all .AC files in TESTS/COMPILER/

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
TEST_DIR="$SCRIPT_DIR/COMPILER"

ASTRAC=""
for CANDIDATE in "$REPO_ROOT/build/AstraC" "$REPO_ROOT/AstraC" "$REPO_ROOT/bin/AstraC"; do
    if [ -f "$CANDIDATE" ] && [ -x "$CANDIDATE" ]; then
        ASTRAC="$CANDIDATE"
        break
    fi
done

if [ -z "$ASTRAC" ]; then
    echo "[ERROR] Could not find AstraC executable. Please build it first."
    exit 1
fi

echo "=================================================="
echo "   AstraC Compiler Test Suite"
echo "   Using: $ASTRAC"
echo "=================================================="

PASSED=0
FAILED=0

for TEST_FILE in "$TEST_DIR"/*.ac; do
    [ -e "$TEST_FILE" ] || continue
    TEST_NAME="$(basename "$TEST_FILE")"
    printf "Running %s ... " "$TEST_NAME"

    BIN_FILE="${TEST_FILE%.ac}.BIN"
    AS_FILE="${TEST_FILE%.ac}.AS"
    rm -f "$BIN_FILE" "$AS_FILE"

    if "$ASTRAC" comp "$TEST_FILE" debug warn 2 > /dev/null 2>&1; then
        if [ -f "$BIN_FILE" ]; then
            echo "PASSED"
            PASSED=$((PASSED + 1))
        else
            echo "FAILED (output binary missing)"
            FAILED=$((FAILED + 1))
        fi
    else
        echo "FAILED (compilation error)"
        FAILED=$((FAILED + 1))
    fi
done

echo "=================================================="
echo "Test Results: $PASSED Passed, $FAILED Failed"
echo "=================================================="

if [ "$FAILED" -gt 0 ]; then
    exit 1
fi
exit 0
