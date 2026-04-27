# nil-service

Python bindings for **nil-service** - a high-performance networking service toolkit for building client/server applications.

## Features

- **Multiple Protocols**: UDP, TCP, WebSocket, HTTP, and pipe support
- **Client & Server Modes**: Flexible role-based service creation
- **Event-Driven API**: Async-friendly event handlers
- **Lightweight**: Minimal dependencies, built on C++ for performance
- **Cross-Platform**: Support for POSIX systems

## Installation

```bash
pip install nil-service
```

## Quick Start

### Basic Server

```python
from nil_service import Service

# Create an HTTP server
server = Service.http_server(port=8000)

@server.on_ready
def on_ready():
    print("Server started")

@server.on_message
def on_message(connection_id, data):
    print(f"Received from {connection_id}: {data}")
    server.send(connection_id, b"Hello back!")

server.run()
```

### WebSocket Server

```python
from nil_service import Service

server = Service.http_server(port=8000)
ws = server.use_ws("/ws")

@ws.on_connect
def on_connect(connection_id):
    print(f"Client {connection_id} connected")

@ws.on_message
def on_message(connection_id, data):
    ws.publish(data)  # Broadcast to all

server.run()
```

### TCP Client

```python
from nil_service import Service

client = Service.tcp_client(host="localhost", port=9000)

@client.on_connect
def on_connect(connection_id):
    client.send(connection_id, b"Hello Server!")

@client.on_message
def on_message(connection_id, data):
    print(f"Received: {data}")

client.run()
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
- `send(connection_id, data)` - Send to a specific connection
- `publish(data)` - Broadcast to all connections

### Event Handlers

- `@service.on_ready` - Service initialized
- `@service.on_connect` - New connection
- `@service.on_disconnect` - Connection closed
- `@service.on_message` - Data received

## Documentation

For detailed API documentation and more examples, visit:
- [C++ Documentation](https://github.com/njaldea/nil-service)
- [C API Guide](https://github.com/njaldea/nil-service/blob/master/docs/c-api.md)

## License

CC BY-NC-ND 4.0

## Support

For issues, questions, or contributions, visit the [GitHub repository](https://github.com/njaldea/nil-service).
