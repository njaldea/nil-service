# nil-service (Lua FFI)

LuaJIT FFI bindings for **nil-service** - a high-performance networking service toolkit for building client/server applications.

## Features

- **Multiple Protocols**: UDP, TCP, WebSocket, HTTP, and pipe support
- **Client & Server Modes**: Flexible role-based service creation
- **Event-Driven API**: Async-friendly event handlers
- **Lightweight**: Minimal dependencies, built on C++ for performance
- **Cross-Platform**: Support for POSIX systems

## Installation

Copy `nil_service.lua` and `libnil-service-c-api.so` into your project, then load the module with `require`.

## Quick Start

### Basic Server

```lua
local Service = require("nil_service")

-- Create an HTTP server
local server = Service.create_http_server("0.0.0.0", 8000, 1024)

server:on_ready(function(id)
    print("Server started")
end)

server:on_message(function(id, data)
    print("Received from " .. id:to_string() .. ": " .. data)
    server:send(id, "Hello back!")
end)

server:run()
```

### WebSocket Server

```lua
local Service = require("nil_service")

local server = Service.create_http_server("0.0.0.0", 8000, 1024)
local ws = server:use_ws("/ws")

ws:on_connect(function(id)
    print("Client " .. id:to_string() .. " connected")
end)

ws:on_message(function(id, data)
    ws:publish(data) -- Broadcast to all
end)

server:run()
```

### TCP Client

```lua
local Service = require("nil_service")

local client = Service.create_tcp_client("127.0.0.1", 9000, 1024)

client:on_connect(function(id)
    client:send(id, "Hello Server!")
end)

client:on_message(function(id, data)
    print("Received: " .. data)
end)

client:run()
```

## Service Types

| Protocol | Mode          | Use Case                    |
|----------|---------------|-----------------------------|
| self     | standalone    | Loopback/echo service       |
| udp      | client/server | Connectionless messaging    |
| tcp      | client/server | Reliable stream transport   |
| ws       | client/server | WebSocket communication     |
| http     | server        | Web server with routing     |
| pipe     | standalone    | POSIX named pipes           |

## API Reference

### Service Methods

- `run()` - Block and run the event loop
- `poll()` - Process pending events (non-blocking)
- `stop()` - Stop the service
- `send(id, data)` - Send to a specific connection
- `publish(data)` - Broadcast to all connections

### Event Handlers

- `on_ready(fn)` - Service initialized
- `on_connect(fn)` - New connection
- `on_disconnect(fn)` - Connection closed
- `on_message(fn)` - Data received

## Data Buffers

Lua strings are treated as byte buffers. Use `string.char(...)` or concatenation to build binary payloads.

## Documentation

For detailed API documentation and more examples, visit:
- [C++ Documentation](https://github.com/njaldea/nil-service)
- [C API Guide](https://github.com/njaldea/nil-service/blob/master/docs/c-api.md)

## License

CC BY-NC-ND 4.0

## Support

For issues, questions, or contributions, visit the [GitHub repository](https://github.com/njaldea/nil-service).
