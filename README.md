# nil/service

This library is only intended to simplify creation of `tcp`/`udp`/`websocket` client/server.

Simplification is done by abstracting the actual implementation of server/client and simply exposing addition of handlers for specific message types.

## Classes

The classes provided by this library conforms in similar API. available protocols are `tcp` | `udp` | `ws`.

### `nil::service::<protocol>:::Server`

- Options

| name    | protocol    | description                              | default   |
| ------- | ----------- | ---------------------------------------- | --------- |
| port    | tcp/udp/ws  | network port to use                      |           |
| buffer  | tcp/udp/ws  | buffer size to use                       | 1024      |
| timeout | udp         | timeuout to consier a connection is lost | 2 seconds |

### `nil::service::<protocol>:::Client`

- Options

| name    | protocol    | description                              | default   |
| ------- | ----------- | ---------------------------------------- | --------- |
| host    | tcp/udp/ws  | network host to connect to               |           |
| port    | tcp/udp/ws  | network port to connect to               |           |
| buffer  | tcp/udp/ws  | buffer size to use                       | 1024      |
| timeout | udp         | timeuout to consier a connection is lost | 2 seconds |

### `nil::service::Self`

- No Options

This service is intended to be just a self invoking event loop.
All messages sent will be sent to itself and forwarded to a message handler.

Messages are still serialized/deserialized.

## methods

```cpp
service.run();      // runs the service - blocking
service.stop();     // stops the service - releases run
service.restart();  // restarts the service 
                    // required after stopping and before re-running

service.on_connect(handler);
service.on_disconnect(handler);
service.on_message(handler);

service.send(id, buffer, buffer_size);
service.send(id, message_with_codec);
service.send(id, vector_of_uint8);

service.publish(buffer, buffer_size);
service.publish(message_with_codec);
service.publish(vector_of_uint8);
```

### `Service::on_message` overloads

- the following callable signatures will be handled:

```cpp
service.on_message(handler);

// the following are possible signatures for the call operator of handler
[](){};
[](const nil::service::ID&){};
[](const nil::service::ID&, const void*, std::uint64_t){};
[](const nil::service::ID&, const WithCodec&){};
[](const void*, std::uint64_t){};
[](const WithCodec&){};

// we can use `auto` for id
[](const auto&){};
[](const auto&, const void*, std::uint64_t){};
[](const auto&, const WithCodec&){};
```

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
void send(nil::service::IService& service)
{
    service.publish(nil::service::concat(0u, "message for 0"s));
    service.publish(nil::service::concat(1u, false));
}
```

This utilizes `codec`s which will be discussesd at the end.

### `split<T>(Handler handler)`

This utility method that will split the payload into two and will create a handler compatible to `on_message`.

- the following callable signatures will be handled:

```cpp
split<T>(handler);

// the following are possible signatures for the call operator of handler
[](const nil::service::ID&, const T&, const void*, std::uint64_t){};
[](const nil::service::ID&, const T&, const WithCodec&){};
[](const T&, const void*, std::uint64_t){};
[](const T&, const WithCodec&){};

// id and T can be auto
[](const auto&, const auto&, const void*, std::uint64_t){};
[](const auto&, const auto&, const WithCodec&){};
[](const auto&, const void*, std::uint64_t){};
[](const auto&, const WithCodec&){};
```

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
#include "my_codec.hpp"
#include <nil/service.hpp>

void setup(nil::service::IService& service)
{
    service.on_message([](const auto& id, const CustomType& message){});
}

void publish(nil::service::IService& service, const CustomType& message)
{
    service.publish(message);
}
```

## NOTES:
- due to the nature of UDP, if one side gets "destroyed" and is able to reconnect "immediately", disconnection will not be "detected".
- (will be fixed in the future) - host is not resolved. currently expects only IP.