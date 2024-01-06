#!/usr/bin/env bash

ROM_FILE="$1"
GOLDEN_LOG="$2"
TIMEOUT="${3:-5s}"
STATE_LOG="state.log"
DEBUG_LOG="debug.log"
BRICKBOY_BIN="./cmake-build-debug/brickboy"

if [[ $ROM_FILE == "" || $GOLDEN_LOG == "" ]]; then
    echo "Usage: $0 <rom_file> <golden_log>"
    exit 1
fi

# Run the emulator and compare the CPU state log with the golden log.
timeout $TIMEOUT $BRICKBOY_BIN --nologo --statelog="${STATE_LOG}" --debuglog="${DEBUG_LOG}" --serial "${ROM_FILE}"
LINECMP_RESULT=$(./scripts/linecmp.py --lineno "$GOLDEN_LOG" "$STATE_LOG")
LINECMP_EXIT_CODE=$?

if [[ $LINECMP_EXIT_CODE == 0 ]]; then
    echo "PASS"
    exit 0
fi

echo "States are different:"
echo "$LINECMP_RESULT"

# Print the disassembly around the error location.
LINE_NUMBER=$(echo "$LINECMP_RESULT" | tail -n 1)
START_LINE=$(($LINE_NUMBER - 10))
END_LINE=$(($LINE_NUMBER))

echo "The error was here:"
#awk "NR >= $START_LINE && NR <= $END_LINE" "$DEBUG_LOG"
sed -n "${START_LINE},${END_LINE}p" "${DEBUG_LOG}"
exit 1
