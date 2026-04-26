#!/bin/bash

set -e

# Get the sandbox root directory
SANDBOX_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$(cd "$SANDBOX_DIR/../.build" && pwd)"

# Check which executables/interpreters are available
SANDBOX_BIN_CPP="${SANDBOX_BIN_CPP:-${SANDBOX_BIN:-$BUILD_DIR/bin/sandbox}}"
SANDBOX_BIN_C="${SANDBOX_BIN_C:-$BUILD_DIR/bin/sandbox-c}"
LUAJIT="${LUAJIT:-luajit}"
PYTHON3="${PYTHON3:-python3}"

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

# Helper function to test a service pair using a given runtime
# Usage: test_service_pair_runtime <runtime_type> <runtime_cmd> <service_type> <service_name>
# runtime_type: "cpp", "c", "lua", "python"
# runtime_cmd: command to run (e.g., "sandbox", "luajit main.lua", "python3 main.py")
# service_type: tcp, udp, ws, pipe
# service_name: human readable name
test_service_pair_runtime() {
    local runtime_type=$1
    local runtime_cmd=$2
    local service_type=$3
    local service_name=$4
    
    # Determine command format based on runtime
    # C++/C: "tcp server" and "-p PORT", Lua/Python: "tcp-server" and just "PORT"
    local cmd_separator=" "
    local port_format="-p"
    if [[ "$runtime_cmd" == *"luajit"* ]] || [[ "$runtime_cmd" == *"python3"* ]]; then
        cmd_separator="-"
        port_format=""
    fi
    
    # Calculate port based on runtime and service type
    # C++: 10000+, C: 20000+, Lua: 30000+, Python: 40000+
    # tcp=0, udp=1, ws=2
    local base_port=0
    local service_offset=0
    
    case "$runtime_type" in
        cpp)   base_port=10000 ;;
        c)     base_port=20000 ;;
        lua)   base_port=30000 ;;
        python) base_port=40000 ;;
    esac
    
    case "$service_type" in
        tcp)   service_offset=0 ;;
        udp)   service_offset=1 ;;
        ws)    service_offset=2 ;;
    esac
    
    local PORT=$((base_port + service_offset))
    
    test_count=$((test_count + 1))
    echo -e "${YELLOW}[TEST $test_count] Testing $service_name ($runtime_type)...${NC}"
    
    local SERVER_LOG=/tmp/${runtime_type}_${service_type}_server.log
    local CLIENT_LOG=/tmp/${runtime_type}_${service_type}_client.log
    local SERVER_FIFO=/tmp/${runtime_type}_${service_type}_server_stdin
    local CLIENT_FIFO=/tmp/${runtime_type}_${service_type}_client_stdin
    
    # Cleanup any lingering processes
    pkill -f "$runtime_cmd" 2>/dev/null || true
    sleep $SLEEP_CLEANUP
    
    # Cleanup
    rm -f "$SERVER_FIFO" "$CLIENT_FIFO" "$SERVER_LOG" "$CLIENT_LOG"
    mkfifo "$SERVER_FIFO"
    mkfifo "$CLIENT_FIFO"
    
    # Start server with fixed port
    if [ -z "$port_format" ]; then
        # Lua/Python: just the port number
        (cd "$BUILD_DIR" && eval "$runtime_cmd ${service_type}${cmd_separator}server $PORT" < "$SERVER_FIFO" > "$SERVER_LOG" 2>&1) &
    else
        # C/C++: -p PORT
        (cd "$BUILD_DIR" && eval "$runtime_cmd ${service_type}${cmd_separator}server $port_format $PORT" < "$SERVER_FIFO" > "$SERVER_LOG" 2>&1) &
    fi
    SERVER_PID=$!
    eval "exec 8>\"$SERVER_FIFO\""  # keep server stdin open
    
    # Give server time to start
    sleep $SLEEP_STARTUP
    
    # Start client with same port
    if [ -z "$port_format" ]; then
        # Lua/Python: just the port number
        (cd "$BUILD_DIR" && eval "$runtime_cmd ${service_type}${cmd_separator}client $PORT" < "$CLIENT_FIFO" > "$CLIENT_LOG" 2>&1) &
    else
        # C/C++: -p PORT
        (cd "$BUILD_DIR" && eval "$runtime_cmd ${service_type}${cmd_separator}client $port_format $PORT" < "$CLIENT_FIFO" > "$CLIENT_LOG" 2>&1) &
    fi
    CLIENT_PID=$!
    eval "exec 9>\"$CLIENT_FIFO\""  # keep client stdin open
    
    sleep $SLEEP_STARTUP
    
    # Check if both are running
    if ! kill -0 $SERVER_PID 2>/dev/null; then
        echo -e "${RED}✗ FAIL${NC}: Server crashed"
        eval "exec 8>&-" 2>/dev/null || true
        eval "exec 9>&-" 2>/dev/null || true
        kill -KILL $CLIENT_PID 2>/dev/null || true
        pkill -f "$runtime_cmd" 2>/dev/null || true
        rm -f "$SERVER_FIFO" "$CLIENT_FIFO"
        fail_count=$((fail_count + 1))
        return 1
    fi
    
    if ! kill -0 $CLIENT_PID 2>/dev/null; then
        echo -e "${RED}✗ FAIL${NC}: Client failed to connect"
        eval "exec 8>&-" 2>/dev/null || true
        eval "exec 9>&-" 2>/dev/null || true
        kill -KILL $SERVER_PID 2>/dev/null || true
        pkill -f "$runtime_cmd" 2>/dev/null || true
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
    eval "exec 8>&-" 2>/dev/null || true
    eval "exec 9>&-" 2>/dev/null || true
    kill -KILL $SERVER_PID $CLIENT_PID 2>/dev/null || true
    wait $SERVER_PID $CLIENT_PID 2>/dev/null || true
    pkill -f "$runtime_cmd" 2>/dev/null || true
    sleep $SLEEP_CLEANUP
    rm -f "$SERVER_FIFO" "$CLIENT_FIFO"
    
    if $PASS; then
        echo -e "${GREEN}✓ PASS${NC}: $service_name ($runtime_type)"
        pass_count=$((pass_count + 1))
    else
        fail_count=$((fail_count + 1))
    fi
}


echo -e "${YELLOW}=== Service Integration Tests ===${NC}\n"

# Test C++ (using sandbox binary)
if [ -x "$SANDBOX_BIN_CPP" ]; then
    echo -e "${YELLOW}--- C++ (native binary) ---${NC}"
    test_service_pair_runtime "cpp" "$SANDBOX_BIN_CPP" "tcp" "TCP Server/Client"
    test_service_pair_runtime "cpp" "$SANDBOX_BIN_CPP" "udp" "UDP Server/Client"
    test_service_pair_runtime "cpp" "$SANDBOX_BIN_CPP" "ws" "WebSocket Server/Client"
else
    echo -e "${YELLOW}--- C++ (native binary) - SKIPPED (binary not found)${NC}"
fi

# Test C (using c-api binary)
if [ -x "$SANDBOX_BIN_C" ]; then
    echo -e "${YELLOW}--- C (c-api) ---${NC}"
    test_service_pair_runtime "c" "$SANDBOX_BIN_C" "tcp" "TCP Server/Client"
    test_service_pair_runtime "c" "$SANDBOX_BIN_C" "udp" "UDP Server/Client"
    test_service_pair_runtime "c" "$SANDBOX_BIN_C" "ws" "WebSocket Server/Client"
else
    echo -e "${YELLOW}--- C (c-api) - SKIPPED (binary not found)${NC}"
fi

# Test Lua
if command -v "$LUAJIT" &> /dev/null; then
    echo -e "\n${YELLOW}--- Lua (LuaJIT) ---${NC}"
    test_service_pair_runtime "lua" "$LUAJIT ../sandbox/main.lua" "tcp" "TCP Server/Client"
    test_service_pair_runtime "lua" "$LUAJIT ../sandbox/main.lua" "udp" "UDP Server/Client"
    test_service_pair_runtime "lua" "$LUAJIT ../sandbox/main.lua" "ws" "WebSocket Server/Client"
else
    echo -e "\n${YELLOW}--- Lua (LuaJIT) - SKIPPED (luajit not found)${NC}"
fi

# Test Python
if command -v "$PYTHON3" &> /dev/null; then
    echo -e "\n${YELLOW}--- Python (ctypes) ---${NC}"
    test_service_pair_runtime "python" "$PYTHON3 ../sandbox/main.py" "tcp" "TCP Server/Client"
    test_service_pair_runtime "python" "$PYTHON3 ../sandbox/main.py" "udp" "UDP Server/Client"
    test_service_pair_runtime "python" "$PYTHON3 ../sandbox/main.py" "ws" "WebSocket Server/Client"
else
    echo -e "\n${YELLOW}--- Python (ctypes) - SKIPPED (python3 not found)${NC}"
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
