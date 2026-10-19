#!/usr/bin/env bash
# AstraC Assembler Feature Test Runner for Linux/macOS
# Tests all .AS files in TESTS/ASSEMBLER/

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
TEST_DIR="$SCRIPT_DIR/ASSEMBLER"

ASTRAC=""
for CANDIDATE in "$REPO_ROOT/build/AstraC" "$REPO_ROOT/AstraC" "$REPO_ROOT/bin/AstraC" "$REPO_ROOT/build/Release/AstraC.exe"; do
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
echo "   AstraC Assembler Test Suite"
echo "   Using: $ASTRAC"
echo "=================================================="

PASSED=0
FAILED=0

for TEST_FILE in "$TEST_DIR"/*.AS; do
    [ -e "$TEST_FILE" ] || continue
    TEST_NAME="$(basename "$TEST_FILE")"
    printf "Running %s ... " "$TEST_NAME"

    BIN_FILE="${TEST_FILE%.AS}.BIN"
    DSM_FILE="${TEST_FILE%.AS}.DSM"
    rm -f "$BIN_FILE" "$DSM_FILE"

    ASM_ARGS=("asm" "$TEST_FILE")
    if [ "$TEST_NAME" = "07_acfh_exe_and_tables.AS" ]; then
        ASM_ARGS+=("type" "exe" "ot" "ft" "it" "entry" "_start")
    fi

    if "$ASTRAC" "${ASM_ARGS[@]}" > /dev/null 2>&1; then
        if [ -f "$BIN_FILE" ]; then
            # Run disassembler
            DISASM_ARGS=("disasm" "$BIN_FILE")
            if [ "$TEST_NAME" = "06_bootloader_realmode.AS" ]; then
                DISASM_ARGS+=("bits" "16" "org" "0x7C00")
            fi
            "$ASTRAC" "${DISASM_ARGS[@]}" > /dev/null 2>&1

            # Check specific conditions
            CHECK_FAILED=0

            if [ "$TEST_NAME" = "06_bootloader_realmode.AS" ]; then
                BIN_SIZE=$(wc -c < "$BIN_FILE")
                if [ "$BIN_SIZE" -ne 512 ]; then
                    CHECK_FAILED=1
                fi
            fi

            if [ "$TEST_NAME" = "07_acfh_exe_and_tables.AS" ]; then
                OBJDUMP_OUT="$("$ASTRAC" objdump "$BIN_FILE" 2>&1)"
                if ! echo "$OBJDUMP_OUT" | grep -q "EXEC RELOCS FUNCS IMPORTS"; then
                    CHECK_FAILED=1
                fi
            fi

            if [ "$CHECK_FAILED" -eq 0 ]; then
                echo "PASSED"
                PASSED=$((PASSED + 1))
            else
                echo "FAILED (verification check failed)"
                FAILED=$((FAILED + 1))
            fi
        else
            echo "FAILED (output binary missing)"
            FAILED=$((FAILED + 1))
        fi
    else
        echo "FAILED (assembler error)"
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
