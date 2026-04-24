#!/bin/bash

set -e

# Use SANDBOX variable if provided, otherwise try to find in PATH or relative to script
SANDBOX="${SANDBOX:-sandbox}"

# If SANDBOX is not found, try relative to this script
if ! command -v "$SANDBOX" &> /dev/null; then
    SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.build/bin" && pwd)"
    SANDBOX="$SCRIPT_DIR/sandbox"
    if [ ! -x "$SANDBOX" ]; then
        echo "Error: sandbox binary not found. Set SANDBOX environment variable." >&2
        exit 1
    fi
fi

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Test configuration
SLEEP_CLEANUP=0.05  # delay after killing processes
SLEEP_STARTUP=0.05  # delay after starting service
SLEEP_OPERATION=0.05  # delay between operations

test_count=0
pass_count=0
fail_count=0

# Generic test function for services with server/client pattern
test_service_pair() {
    local service_type=$1  # tcp, udp, ws
    local service_name=$2  # Human readable name
    
    test_count=$((test_count + 1))
    echo -e "${YELLOW}[TEST $test_count] Testing $service_name...${NC}"
    
    local SERVER_LOG=/tmp/${service_type}_server.log
    local CLIENT_LOG=/tmp/${service_type}_client.log
    local SERVER_FIFO=/tmp/${service_type}_server_stdin
    local CLIENT_FIFO=/tmp/${service_type}_client_stdin
    
    # Cleanup any lingering processes
    pkill -f "sandbox $service_type" 2>/dev/null || true
    sleep $SLEEP_CLEANUP
    
    # Cleanup
    rm -f "$SERVER_FIFO" "$CLIENT_FIFO" "$SERVER_LOG" "$CLIENT_LOG"
    mkfifo "$SERVER_FIFO"
    mkfifo "$CLIENT_FIFO"
    
    # Start server with port 0 (dynamic assignment)
    "$SANDBOX" $service_type server < "$SERVER_FIFO" > "$SERVER_LOG" 2>&1 &
    SERVER_PID=$!
    eval "exec 8>\"$SERVER_FIFO\""  # keep server stdin open
    
    # Give server time to start and output the assigned port
    sleep $SLEEP_STARTUP
    
    # Extract the port from server output: "local        : 127.0.0.1:PORT"
    local PORT=$(grep -oP '(?<=:)\d+' "$SERVER_LOG" 2>/dev/null | head -1)
    
    if [ -z "$PORT" ]; then
        echo -e "${RED}✗ FAIL${NC}: Could not extract port from server output"
        eval "exec 8>&-"
        kill -KILL $SERVER_PID 2>/dev/null || true
        pkill -f "sandbox $service_type" 2>/dev/null || true
        rm -f "$SERVER_FIFO" "$CLIENT_FIFO"
        fail_count=$((fail_count + 1))
        return 1
    fi
    
    # Start client
    "$SANDBOX" $service_type client -p $PORT < "$CLIENT_FIFO" > "$CLIENT_LOG" 2>&1 &
    CLIENT_PID=$!
    eval "exec 9>\"$CLIENT_FIFO\""  # keep client stdin open
    
    sleep $SLEEP_STARTUP
    
    # Check if both are running
    if ! kill -0 $SERVER_PID 2>/dev/null; then
        echo -e "${RED}✗ FAIL${NC}: Server crashed"
        eval "exec 8>&-"
        eval "exec 9>&-" 2>/dev/null || true
        kill -KILL $CLIENT_PID 2>/dev/null || true
        pkill -f "sandbox $service_type" 2>/dev/null || true
        rm -f "$SERVER_FIFO" "$CLIENT_FIFO"
        fail_count=$((fail_count + 1))
        return 1
    fi
    
    if ! kill -0 $CLIENT_PID 2>/dev/null; then
        echo -e "${RED}✗ FAIL${NC}: Client failed to connect"
        eval "exec 8>&-"
        eval "exec 9>&-" 2>/dev/null || true
        kill -KILL $SERVER_PID 2>/dev/null || true
        pkill -f "sandbox $service_type" 2>/dev/null || true
        rm -f "$SERVER_FIFO" "$CLIENT_FIFO"
        fail_count=$((fail_count + 1))
        return 1
    fi
    
    # Send message from client to server
    echo "msg_from_client" >&9
    sleep $SLEEP_OPERATION
    
    # Send message from server to client
    echo "msg_from_server" >&8
    sleep $SLEEP_OPERATION
    
    # Verify messages were received
    local PASS=true
    
    if ! grep -q "connected" "$SERVER_LOG"; then
        echo -e "${RED}✗ FAIL${NC}: Server did not see connection"
        PASS=false
    fi
    
    if ! grep -q "connected" "$CLIENT_LOG"; then
        echo -e "${RED}✗ FAIL${NC}: Client did not connect"
        PASS=false
    fi
    
    if ! grep -q "msg_from_client" "$SERVER_LOG"; then
        echo -e "${RED}✗ FAIL${NC}: Server did not receive client message"
        PASS=false
    fi
    
    if ! grep -q "msg_from_server" "$CLIENT_LOG"; then
        echo -e "${RED}✗ FAIL${NC}: Client did not receive server message"
        PASS=false
    fi
    
    # Cleanup
    eval "exec 8>&-"
    eval "exec 9>&-" 2>/dev/null || true
    kill -KILL $SERVER_PID $CLIENT_PID 2>/dev/null || true
    wait $SERVER_PID $CLIENT_PID 2>/dev/null || true
    pkill -f "sandbox $service_type" 2>/dev/null || true
    sleep $SLEEP_CLEANUP
    rm -f "$SERVER_FIFO" "$CLIENT_FIFO"
    
    if $PASS; then
        echo -e "${GREEN}✓ PASS${NC}: $service_name"
        pass_count=$((pass_count + 1))
    else
        fail_count=$((fail_count + 1))
    fi
}

echo -e "${YELLOW}=== Service Integration Tests ===${NC}\n"

# Test TCP
test_service_pair "tcp" "TCP Server/Client"

# Test UDP
test_service_pair "udp" "UDP Server/Client"

# Test WebSocket
test_service_pair "ws" "WebSocket Server/Client"

# Test Pipe (uses two instances with -f for second)
test_count=$((test_count + 1))
echo -e "${YELLOW}[TEST $test_count] Testing Pipe Service Pair...${NC}"

PIPE_READER_LOG=/tmp/pipe_reader.log
PIPE_WRITER_LOG=/tmp/pipe_writer.log
PIPE_READER_STDIN=/tmp/pipe_reader_stdin
PIPE_WRITER_STDIN=/tmp/pipe_writer_stdin

# Cleanup any lingering processes
pkill -f "sandbox pipe" 2>/dev/null || true
sleep $SLEEP_CLEANUP

# Cleanup
rm -f "$PIPE_READER_STDIN" "$PIPE_WRITER_STDIN" "$PIPE_READER_LOG" "$PIPE_WRITER_LOG"
mkfifo "$PIPE_READER_STDIN"
mkfifo "$PIPE_WRITER_STDIN"

# Start reader (first terminal)
"$SANDBOX" pipe < "$PIPE_READER_STDIN" > "$PIPE_READER_LOG" 2>&1 &
READER_PID=$!
eval "exec 8>\"$PIPE_READER_STDIN\""

sleep $SLEEP_STARTUP

# Start writer with -f flag (second terminal)
"$SANDBOX" pipe -f < "$PIPE_WRITER_STDIN" > "$PIPE_WRITER_LOG" 2>&1 &
WRITER_PID=$!
eval "exec 9>\"$PIPE_WRITER_STDIN\""

sleep $SLEEP_STARTUP

# Check if both are running
PIPE_PASS=true

if ! kill -0 $READER_PID 2>/dev/null; then
    echo -e "${RED}✗ FAIL${NC}: Pipe reader crashed"
    PIPE_PASS=false
fi

if ! kill -0 $WRITER_PID 2>/dev/null; then
    echo -e "${RED}✗ FAIL${NC}: Pipe writer failed"
    PIPE_PASS=false
fi

# Verify both see connection
if ! grep -q "connected" "$PIPE_READER_LOG"; then
    echo -e "${RED}✗ FAIL${NC}: Pipe reader did not see connection"
    PIPE_PASS=false
fi

if ! grep -q "connected" "$PIPE_WRITER_LOG"; then
    echo -e "${RED}✗ FAIL${NC}: Pipe writer did not see connection"
    PIPE_PASS=false
fi

# Cleanup
eval "exec 8>&-"
eval "exec 9>&-" 2>/dev/null || true
kill -KILL $READER_PID $WRITER_PID 2>/dev/null || true
wait $READER_PID $WRITER_PID 2>/dev/null || true
pkill -f "sandbox pipe" 2>/dev/null || true
sleep $SLEEP_CLEANUP
rm -f "$PIPE_READER_STDIN" "$PIPE_WRITER_STDIN"

if $PIPE_PASS; then
    echo -e "${GREEN}✓ PASS${NC}: Pipe Service Pair"
    pass_count=$((pass_count + 1))
else
    fail_count=$((fail_count + 1))
fi

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
