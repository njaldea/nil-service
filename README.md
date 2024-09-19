# nil/service

This library is only intended to simplify creation of different services for network communincation.

Here are the services provided by this library:

| name   | description                                                                       |
| ------ | --------------------------------------------------------------------------------- |
| _self_ | works like an echo server. messages sent/published is handled by its own handlers |
| _udp_  | client/server                                                                     |
| _tcp_  | client/server                                                                     |
| _ws_   | client/server                                                                     |
| _http_ | server. supports routes and promotion to websocket. Current API is not final.     |

## Options

The following are properties of the Option struct required for creating the services.

### `<protocol>::server::Options`

| name    | protocol         | description                              | default   |
| ------- | ---------------- | ---------------------------------------- | --------- |
| port    | tcp/udp/ws/http  | network port to use                      |           |
| buffer  | tcp/udp/ws/http  | buffer size to use                       | 1024      |
| timeout | udp              | timeout to consider a connection is lost | 2 seconds |

### `<protocol>::client::Options`

| name    | protocol    | description                              | default   |
| ------- | ----------- | ---------------------------------------- | --------- |
| host    | tcp/udp/ws  | network host to connect to               |           |
| port    | tcp/udp/ws  | network port to connect to               |           |
| buffer  | tcp/udp/ws  | buffer size to use                       | 1024      |
| timeout | udp         | timeuout to consier a connection is lost | 2 seconds |

## Creating The Services

Each service has their own create method.

```cpp
namespace ns = nil::service;
auto a = ns::self::create();
auto a = ns::udp::server::create({...});
auto a = ns::udp::client::create({...});
auto a = ns::tcp::server::create({...});
auto a = ns::tcp::client::create({...});
auto a = ns::ws::server::create({...});
auto a = ns::ws::client::create({...});
auto s = ns::http::server::create({...});
```

### Proxy Types

The create methods returns a proxy object that is responsible for the following:
- owning the object and managing its lifetime
- conversion to different types to allow necessary conversion to matching supported API

There are 3 types of Proxies:

| type | description               | observable    | runnable | messaging |
| ---- | ------------------------- | ------------- | -------- | --------- |
| _A_  | standalone service proxy  | yes           | yes      | yes       |
| _H_  | http service proxy        | on_ready      | yes      | no        |
| _S_  | service proxy             | yes           | no       | yes       |

## APIs

### Service

```cpp
on_ready(service, handler);
on_connect(service, handler);
on_disconnect(service, handler);
on_message(service, handler);

send(service, id, buffer, buffer_size);
send(service, id, message_with_codec);
send(service, id, vector_of_uint8);

publish(service, buffer, buffer_size);
publish(service, message_with_codec);
publish(service, vector_of_uint8);
```

### Standalone Service

Standalone service also support the basic Service methods.

```cpp
auto service = nil::service::...::create({...});

start(service);    // runs the service - blocking
stop(service);     // stops the service - releases run
restart(service);  // restarts the service 
                   // required after stopping and before re-running

```

### HTTP Service

```cpp
auto service = nil::service::http::server::create({...});

// only supports on_ready
on_ready(service, handler);

// special api for creating routes
use(
    service,
    "/route/here",
    "text/html",
    [](std::ostream& oss) { oss << "your html here"; }
);

// special api for creating route with websocket.
// returns a Service (S proxy)
auto s = use_ws(service, "/ws");
```

### `on_message` overloads

- the following callable signatures will be handled:

```cpp
on_message(service, handler);

// the following are possible signatures for the call operator of handler
[](){};
[](const nil::service::ID& id){};
[](const nil::service::ID& id, const void* buffer, std::uint64_t size){};
[](const nil::service::ID& id, const WithCodec& custom_data){};
[](const void* buffer, std::uint64_t size){};
[](const WithCodec& custom_data){};

// we can use `auto` for id
// use `const auto&` or other variants. up to you.
[](auto id){};
[](auto id, const void* buffer, std::uint64_t size){};
[](auto id, const WithCodec&){};

// use auto will automatically be deduced
[](auto id, auto data, auto raw){};

// custom types can't be deduced with auto
[](auto id, auto custom_data){}; // custom_data's type is unknown
[](auto arg){}; // arg is going to be deduced as id
```

Preferrably, it is better to use distinct types to avoid confusion. but you do you.

### `consume<T>(data, size)`

A utility method to simplify consumption of data and size to an appropriate type.
This is expected to move the data pointer and size depending on how much of the buffer is used.

```cpp
const auto payload = nil::service::consume<std::uint64_t>(data, size);
// sizeof(std::uint64_t) == 8
// data should have moved by 8 bytes
// size should have been incremented by 8
```

This utilizes `codec`s which will be discussesd at the end.

### `concat(T...)`

This utility method provides a way to concatenate multiple payloads into one.

```cpp
publish(service, nil::service::concat(0u, "message for 0"s));
publish(service, nil::service::concat(1u, false));
```

This utilizes `codec`s which will be discussesd at the end.

### `map<T, H...>(mapping<T, H>... handler)`

This utility method creates a handler that will do the following:
- parses the first section of the buffer (like having a header)
- calls the right mapping (callback) based on the header
- returns a handler compatible to `on_message`

```cpp
T header1;
T header2;
map(
    mapping(header1, handler1),
    mapping(header2, handler2)
);
```

Notes:
- all mappings inside the map should have the same header type.
- handlers will follow the same signature support as `on_message`.

### `codec`

to allow serialization of custom type (message for send/publish), `codec` is expected to be implemented by the user.

the following codecs are already implemented:

```cpp
codec<std::string>
codec<std::uint8_t>
codec<std::uint16_t>
codec<std::uint32_t>
codec<std::uint64_t>
codec<std::int8_t>
codec<std::int16_t>
codec<std::int32_t>
codec<std::int64_t>
```

a `codec` is expected to have `serialize` and `deserialize` method. see example below for more information.

```cpp
// my_codec.hpp

#include <nil/service/codec.hpp>
#include <nil/service/consume.hpp>
#include <nil/service/concat.hpp>

struct CustomType
{
    std::int64_t content_1;
    std::int32_t content_2;
    std::int32_t content_3;
};

namespace nil::service
{
    template <>
    struct codec<CustomType>
    {
        static std::vector<std::uint8_t> serialize(const CustomType& message)
        {
            // nil::service::concat
            return concat(
                message.content_1,
                message.content_2,
                message.content_3
            );
        }

        static CustomType deserialize(const void* data, std::uint64_t& size)
        {
            // nil::service::consume
            CustomType m;
            m.content_1 = consume<std::int64_t>(data, size);
            m.content_2 = consume<std::int64_t>(data, size);
            m.content_3 = consume<std::int64_t>(data, size);
            return m;
        }
    };
}

// Include your codec

on_message(service, [](const auto& id, const CustomType& message){});

CustomType message;
publish(service, message);
```

## NOTES:
- due to the nature of UDP, if one side gets "destroyed" and is able to reconnect "immediately", disconnection will not be "detected".
- (will be fixed in the future) - host is not resolved. currently expects only IP.