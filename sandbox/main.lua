package.path = package.path .. ";../src/ffi/lua/?.lua"

local nil_service = require("nil_service")
local ffi = require("ffi")

-- FFI definitions for non-blocking stdin
ffi.cdef [[
    int fcntl(int fd, int cmd, int flags);
    ssize_t read(int fd, void *buf, size_t count);
    int usleep(unsigned int useconds);
    int errno;
]]

-- Constants
local F_SETFL = 4
local O_NONBLOCK = 2048

-- Set stdin to non-blocking mode
local function set_nonblocking()
    local flags = ffi.C.fcntl(0, 4, 2048)  -- fcntl(0, F_SETFL, O_NONBLOCK)
    return flags >= 0
end

-- Read a line from stdin without blocking
local line_buffer = ""
local function read_line_nonblocking()
    local buf = ffi.new("char[1]")
    
    while true do
        local n = ffi.C.read(0, buf, 1)
        
        if n > 0 then
            -- We got a character
            local char = string.char(buf[0])
            
            if char == "\n" then
                -- End of line
                local result = line_buffer
                line_buffer = ""
                return result
            else
                -- Add to buffer
                line_buffer = line_buffer .. char
            end
        elseif n == 0 then
            -- EOF
            return nil
        else
            -- n < 0, EAGAIN or other error - no data available
            return nil
        end
    end
end

-- Initialize non-blocking mode
set_nonblocking()

---@param service nil_service.Event
local function setup_handlers(service)
    service:on_ready(function(id)
        print("local        : " .. id:to_string())
    end)

    service:on_connect(function(id)
        print("connected    : " .. id:to_string())
    end)

    service:on_disconnect(function(id)
        print("disconnected : " .. id:to_string())
    end)

    service:on_message(function(id, data)
        print("from         : " .. id:to_string())
        print("type         : " .. string.sub(data, 1, 1))
        print("message      : " .. string.sub(data, 2))
    end)
end

---@param service nil_service.Standalone | nil_service.Gateway
local function input_output(service)
    local type_flag = 0

    io.write("> ")
    io.flush()
    
    while true do
        -- Poll service to process events
        service:poll()
        
        -- Try to read a line without blocking
        local message = read_line_nonblocking()
        
        if message then
            if message == "reconnect" then
                break
            end

            local prefix = type_flag == 0 and "A" or "B"
            local full_message = prefix .. "typed > " .. message .. " : secondary here"

            service:publish(full_message)
            type_flag = (type_flag + 1) % 2
            
            io.write("> ")
            io.flush()
        else
            -- No input available, sleep a bit before polling again
            ffi.C.usleep(1000)  -- 1ms
        end
    end
end

---@param port number
local function run_tcp_server(port)
    print("Starting TCP server on port " .. port)

    local service = nil_service.create_tcp_server("127.0.0.1", port, 1024 * 1024) ---@type nil_service.Standalone
    setup_handlers(service)

    print("Running TCP server... (type messages or 'reconnect' to exit)")

    input_output(service)
    service:stop()
    service:destroy()
end

---@param port number
local function run_udp_server(port)
    print("Starting UDP server on port " .. port)

    local service = nil_service.create_udp_server("127.0.0.1", port, 1024 * 1024) ---@type nil_service.Standalone
    setup_handlers(service)

    print("Running UDP server... (type messages or 'reconnect' to exit)")

    input_output(service)
    service:stop()
    service:destroy()
end

---@param port number
---@param route string
local function run_ws_server(port, route)
    print("Starting WebSocket server on port " .. port .. " at " .. route)

    local service = nil_service.create_ws_server("127.0.0.1", port, route, 1024 * 1024) ---@type nil_service.Standalone
    setup_handlers(service)

    print("Running WebSocket server... (type messages or 'reconnect' to exit)")

    input_output(service)
    service:stop()
    service:destroy()
end

local function run_gateway()
    print("Starting Gateway with TCP, UDP, and WebSocket services")

    local tcp = nil_service.create_tcp_server("127.0.0.1", 9000, 1024 * 1024) ---@type nil_service.Standalone
    local udp = nil_service.create_udp_server("127.0.0.1", 9001, 1024 * 1024) ---@type nil_service.Standalone
    local ws = nil_service.create_ws_server("127.0.0.1", 9002, "/", 1024 * 1024) ---@type nil_service.Standalone

    local gateway = nil_service.create_gateway()

    -- Add all services to gateway
    gateway:add_service(tcp)
    gateway:add_service(udp)
    gateway:add_service(ws)

    -- Setup handlers on gateway
    setup_handlers(gateway)

    print("Running Gateway... (type messages or 'reconnect' to exit)")

    input_output(gateway)
    gateway:stop()

    tcp:destroy()
    udp:destroy()
    ws:destroy()
    gateway:destroy()
end

local function run_tcp_client(port)
    print("Starting TCP client connecting to port " .. port)

    local service = nil_service.create_tcp_client("127.0.0.1", port, 1024 * 1024) ---@type nil_service.Standalone
    setup_handlers(service)

    print("Running TCP client... (type messages or 'reconnect' to exit)")

    input_output(service)
    service:stop()
    service:destroy()
end

local function run_udp_client(port)
    print("Starting UDP client connecting to port " .. port)

    local service = nil_service.create_udp_client("127.0.0.1", port, 1024 * 1024) ---@type nil_service.Standalone
    setup_handlers(service)

    print("Running UDP client... (type messages or 'reconnect' to exit)")

    input_output(service)
    service:stop()
    service:destroy()
end

local function run_ws_client(port, route)
    print("Starting WebSocket client connecting to port " .. port .. " at " .. route)

    local service = nil_service.create_ws_client("127.0.0.1", port, route, 1024 * 1024) ---@type nil_service.Standalone
    setup_handlers(service)

    print("Running WebSocket client... (type messages or 'reconnect' to exit)")

    input_output(service)
    service:stop()
    service:destroy()
end

---@param port number
local function run_http_server(port)
    print("Starting HTTP server on port " .. port)

    local web = nil_service.create_http_server("0.0.0.0", port, 100 * 1024 * 1024) ---@type nil_service.Web
    
    web:on_ready(function(id)
        print("HTTP server ready: " .. id:to_string())
    end)

    web:on_get(function(transaction)
        ---@cast transaction nil_service.WebTransaction
        local route = transaction:get_route()
        if route == "/" then
            transaction:set_content_type("text/html")
            transaction:send(
                "<!DOCTYPE html>" ..
                "<html lang=\"en\">" ..
                "<head>" ..
                "<title>nil-service HTTP</title>" ..
                "</head>" ..
                "<body><h1>Hello from nil-service Lua FFI</h1></body>" ..
                "</html>"
            )
            return true
        end
        return false
    end)

    local ws_event = web:use_ws("/ws")
    setup_handlers(ws_event)

    print("Running HTTP server... (press Ctrl+C to exit)")

    -- Simple polling loop
    for i = 1, 100 do
        web:poll()
        require("ffi").C.usleep(100000)  -- 100ms sleep
    end

    web:destroy()
end

local function print_help()
    print("Usage: lua main.lua <command> [options]")
    print("")
    print("Server Commands:")
    print("  tcp-server [port]       Run TCP server (default port: 9000)")
    print("  udp-server [port]       Run UDP server (default port: 9001)")
    print("  ws-server [port] [route] Run WebSocket server (default: 9002, /)")
    print("  http-server [port]      Run HTTP server (default port: 8080)")
    print("  gateway                 Run Gateway with all protocols")
    print("")
    print("Client Commands:")
    print("  tcp-client [port]       Run TCP client (default port: 9000)")
    print("  udp-client [port]       Run UDP client (default port: 9001)")
    print("  ws-client [port] [route] Run WebSocket client (default: 9002, /)")
    print("")
    print("Other Commands:")
    print("  help                    Show this help message")
end

-- Main entry point
local args = {...}
local command = args[1] or "help"

if command == "tcp-server" then
    run_tcp_server(tonumber(args[2]) or 9000)
elseif command == "tcp-client" then
    run_tcp_client(tonumber(args[2]) or 9000)
elseif command == "udp-server" then
    run_udp_server(tonumber(args[2]) or 9001)
elseif command == "udp-client" then
    run_udp_client(tonumber(args[2]) or 9001)
elseif command == "ws-server" then
    run_ws_server(tonumber(args[2]) or 9002, args[3] or "/")
elseif command == "ws-client" then
    run_ws_client(tonumber(args[2]) or 9002, args[3] or "/")
elseif command == "gateway" then
    run_gateway()
elseif command == "http-server" then
    run_http_server(tonumber(args[2]) or 8080)
else
    print_help()
end
