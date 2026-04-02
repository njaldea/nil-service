# nil/service

Networking service toolkit for C++ with an optional C API.

## What You Get

| Protocol | Role | Notes |
| --- | --- | --- |
| self | standalone | loopback/echo-style local service |
| udp | client/server | supports timeout |
| tcp | client/server | stream transport |
| ws | client/server | websocket over tcp |
| http | server | routes + websocket upgrade |

## Documentation Map

- C++ API and utilities: this file
- C API guide: [docs/c-api.md](docs/c-api.md)
- Sandbox examples: [sandbox/main.cpp](sandbox/main.cpp), [sandbox/main.c](sandbox/main.c)

## Service Creation (C++)

```cpp
namespace ns = nil::service;

auto self = ns::self::create();
auto udp_server = ns::udp::server::create({...});
auto udp_client = ns::udp::client::create({...});
auto tcp_server = ns::tcp::server::create({...});
auto tcp_client = ns::tcp::client::create({...});
auto ws_server = ns::ws::server::create({...});
auto ws_client = ns::ws::client::create({...});
auto http_server = ns::http::server::create({...});
```

## Service APIs (C++)

### Event service

```cpp
service->on_ready(handler);
service->on_connect(handler);
service->on_disconnect(handler);
service->on_message(handler);

service->send(id, buffer, size);
service->publish(buffer, size);
```

### Standalone service

```cpp
service->start();
service->stop();
service->restart();
```

### Web service

```cpp
auto web = nil::service::http::server::create({...});

web->on_ready(handler);
web->on_get(
    [](nil::service::WebTransaction& tx)
    {
        if ("/" != get_route(tx))
        {
            return false;
        }

        set_content_type(tx, "text/html");
        send(tx, "<body>hello</body>");
        return true;
    }
);

nil::service::IEventService* ws = web->use_ws("/ws");
```

## Options Summary

### server::Options

| Field | Protocols | Notes |
| --- | --- | --- |
| host | tcp, udp, ws, http | bind address |
| port | tcp, udp, ws, http | bind port |
| buffer | tcp, udp, ws, http | io buffer size |
| route | ws | websocket route, default "/" |
| timeout | udp | disconnect timeout, default 2s |

### client::Options

| Field | Protocols | Notes |
| --- | --- | --- |
| host | tcp, udp, ws | target address |
| port | tcp, udp, ws | target port |
| route | ws | websocket route, default "/" |
| buffer | tcp, udp, ws | io buffer size |
| timeout | udp | disconnect timeout, default 2s |

### Default Values

- tcp client/server buffer: `1024`
- udp client/server buffer: `1024`, timeout: `2s`
- ws client/server route: `/`, buffer: `1024`
- http server buffer: `8192`

## on_message Signatures

Supported handler shapes include:

```cpp
[](){};
[](nil::service::ID){};
[](nil::service::ID, const void*, std::uint64_t){};
[](nil::service::ID, const CustomType&){};
[](const void*, std::uint64_t){};
[](const CustomType&){};
```

`auto` forms are also supported by the adapter utilities.

## Utility Helpers

### consume<T>

Consumes bytes from `(data, size)` and advances both according to `codec<T>`.

### concat / concat_into

Serialize one or more values into a contiguous payload using `codec<T>`.

### map / mapping

Build message routers using a header value and per-header handlers.

### codec<T>

Provide `size`, `serialize`, and `deserialize` to integrate custom payload types.

## Lifetime And Thread-Safety Notes

- `nil::service::to_string(ID)` is valid only while handling the callback that supplied that id.
- `ID::to_string` / `to_string(ID)` is not thread-safe.

## Build Notes

- C API is built when `ENABLE_C_API` is ON.
- See [src/CMakeLists.txt](src/CMakeLists.txt) for target details.

## Operational Notes

- Hostnames are not resolved internally; use ip/port values.
- With UDP, very fast reconnect scenarios may skip observable disconnect events.
