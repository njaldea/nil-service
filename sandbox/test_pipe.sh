#!/bin/bash

set -e

# Get the sandbox root directory
SANDBOX_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$(cd "$SANDBOX_DIR/../.build" && pwd)"

# Check which executables/interpreters are available
SANDBOX_BIN_CPP="${SANDBOX_BIN_CPP:-${SANDBOX_BIN:-$BUILD_DIR/bin/sandbox}}"

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Test configuration
SLEEP_CLEANUP=0.05  # delay after killing processes
SLEEP_STARTUP=0.05  # delay after starting service

test_count=0
pass_count=0
fail_count=0

test_pipe_pair() {
    test_count=$((test_count + 1))
    echo -e "${YELLOW}[TEST $test_count] Testing Pipe Service Pair...${NC}"

    local PIPE_READER_LOG=/tmp/pipe_reader.log
    local PIPE_WRITER_LOG=/tmp/pipe_writer.log
    local PIPE_READER_STDIN=/tmp/pipe_reader_stdin
    local PIPE_WRITER_STDIN=/tmp/pipe_writer_stdin

    # Cleanup any lingering processes
    pkill -f "sandbox pipe" 2>/dev/null || true
    sleep $SLEEP_CLEANUP

    rm -f "$PIPE_READER_STDIN" "$PIPE_WRITER_STDIN" "$PIPE_READER_LOG" "$PIPE_WRITER_LOG"
    mkfifo "$PIPE_READER_STDIN"
    mkfifo "$PIPE_WRITER_STDIN"

    # Start reader (first terminal)
    "$SANDBOX_BIN_CPP" pipe < "$PIPE_READER_STDIN" > "$PIPE_READER_LOG" 2>&1 &
    local READER_PID=$!
    eval "exec 8>\"$PIPE_READER_STDIN\""

    sleep $SLEEP_STARTUP

    # Start writer with -f flag (second terminal)
    "$SANDBOX_BIN_CPP" pipe -f < "$PIPE_WRITER_STDIN" > "$PIPE_WRITER_LOG" 2>&1 &
    local WRITER_PID=$!
    eval "exec 9>\"$PIPE_WRITER_STDIN\""

    sleep $SLEEP_STARTUP

    local PASS=true

    if ! kill -0 $READER_PID 2>/dev/null; then
        echo -e "${RED}✗ FAIL${NC}: Pipe reader crashed"
        PASS=false
    fi

    if ! kill -0 $WRITER_PID 2>/dev/null; then
        echo -e "${RED}✗ FAIL${NC}: Pipe writer failed"
        PASS=false
    fi

    if ! grep -q "connected" "$PIPE_READER_LOG"; then
        echo -e "${RED}✗ FAIL${NC}: Pipe reader did not see connection"
        PASS=false
    fi

    if ! grep -q "connected" "$PIPE_WRITER_LOG"; then
        echo -e "${RED}✗ FAIL${NC}: Pipe writer did not see connection"
        PASS=false
    fi

    # Cleanup
    eval "exec 8>&-" 2>/dev/null || true
    eval "exec 9>&-" 2>/dev/null || true
    kill -KILL $READER_PID $WRITER_PID 2>/dev/null || true
    wait $READER_PID $WRITER_PID 2>/dev/null || true
    pkill -f "sandbox pipe" 2>/dev/null || true
    sleep $SLEEP_CLEANUP
    rm -f "$PIPE_READER_STDIN" "$PIPE_WRITER_STDIN"

    if $PASS; then
        echo -e "${GREEN}✓ PASS${NC}: Pipe Service Pair"
        pass_count=$((pass_count + 1))
    else
        fail_count=$((fail_count + 1))
    fi
}

echo -e "${YELLOW}=== Pipe Integration Tests ===${NC}\n"

test_pipe_pair

echo -e "\n${YELLOW}=== Summary ===${NC}"
echo -e "Total:  $test_count"
echo -e "${GREEN}Passed: $pass_count${NC}"
echo -e "${RED}Failed: $fail_count${NC}"

if [ $fail_count -eq 0 ]; then
    echo -e "\n${GREEN}All tests passed!${NC}"
    exit 0
else
    echo -e "\n${RED}Some tests failed!${NC}"
    exit 1
fi
