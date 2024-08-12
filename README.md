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

### methods

| name                             | description                                |
| -------------------------------- | ------------------------------------------ |
| `run()`                          | runs the service                           |
| `stop()`                         | signals to stop the service                |
| `restart()`                      | restart service after stop                 |
| `on_connect(handler)`            | register connect handler                   |
| `on_disconnect(handler)`         | register disconnect handler                |
| `on_message(handler)`            | register message handler                   |
| `send(id, data, size)`           | send message to a specific id/connection   |
| `publish(data, size)`            | send message to all connection             |
| `send(id, message)`              | send message to a specific id/connection   |
| `publish(message)`               | send message to all connection             |

### `send`/`publish` arguments

- by default only accepts the buffer typed as `std::vector<std::uint8_t>`
- if message is not of type `std::vector<std::uint8_t>`, codec serialization will be used.

### `Service::on_message` overloads

- the callable passed to these methods should have the following arguments:
    -  const std::string&   - for the id
    -  const void*          - for the data
    -  std::uint64_t        - for the data size
- the following will be implicitly handled in order of evaluation:
    -  no arguments
    -  only id
    -  id with `const void*` and `size`
    -  id with a custom type (with codec definition)
    -  no id but with `const void*` and `size`
    -  no id but with a custom type (with codec definition)

### `type_cast<T>(data, size)`

A utility method to simplify consumption of data and size to an appropriate type.
This is expected to move the data pointer and size depending on how much of the buffer is used.

```cpp
const auto payload = nil::service::type_cast<std::uint64_t>(data, size);
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

This utility method wraps a handler to provide easier conversion of the data
- first section is the type provided
- second section is the actual payload

This allows re-mapping of different callbacks for different index.

The example below is done via manual parsing:

```cpp
void add_handlers(nil::service::IService& service)
{
    service.on_message(
        [](
            const nil::service::ID&, // optional
            const void* data,
            std::uint64_t size
        )
        {
            const auto tag = nil::service::type_cast<std::uint32_t>(data, size);
            switch (tag)
            {
                case 0:
                    const auto payload = 
                        nil::service::type_cast<std::string>(data, size);
                    break;
                case 1:
                    const auto payload =
                        nil::service::type_cast<bool>(data, size);
                    break;
            }
        }
    );
}
```

The example below allows parsing of the message sent by concat from previous section:

```cpp
void add_handlers(nil::service::IService& service)
{
    service.on_message(
        nil::service::split<std::uint32_t>(
            [](
                const nil::service::ID&, // optional
                std::uint32_t tag,
                const void* data,
                std::uint64_t size
            )
            {
                switch (tag)
                {
                    case 0:
                        const auto payload = 
                            nil::service::type_cast<std::string>(data, size);
                        break;
                    case 1:
                        const auto payload =
                            nil::service::type_cast<bool>(data, size);
                        break;
                }
            }
        )
    );
}
```

### `codec`

to allow serialization of custom type (message for send/publish), `codec` is expected to be implemented by the user.

the following codecs are already implemented:
- `codec<std::string>`
- `codec<std::uint8_t>`
- `codec<std::uint16_t>`
- `codec<std::uint32_t>`
- `codec<std::uint64_t>`
- `codec<std::int8_t>`
- `codec<std::int16_t>`
- `codec<std::int32_t>`
- `codec<std::int64_t>`

a `codec` is expected to have `serialize` and `deserialize` method. see example below for more information.

```cpp
// codec definition (hpp)

// include this header for base template
#include <nil/service/codec.hpp>

#include <google/protobuf/message_lite.h>

namespace nil::service
{
    template <typename Message>
    constexpr auto is_message_lite = std::is_base_of_v<google::protobuf::MessageLite, Message>;

    template <typename Message>
    struct codec<Message, std::enable_if_t<is_message_lite<Message>>>
    {
        static std::vector<std::uint8_t> serialize(const Message& message)
        {
            return codec<std::string>::serialize(message.SerializeAsString());
        }

        // size is the number of available bytes to read from data
        // size must be adjusted to the right value after consuming portion of the data
        // for example, if the data to deserialize is 100, 100 must be deducted from size
        // > size -= 100;
        static Message deserialize(const void* data, std::uint64_t& size)
        {
            Message m;
            m.ParseFromArray(data, int(size));
            size = 0;
            return m;
        }
    };
}

// consuming cpp

// Include your codec
#include "my_codec.hpp"
#include <nil/service.hpp>

template <typename ProtobufMessage>
void foo(nil::service::IService& service, const ProtobufMessage& message)
{
    service.publish(message);
}
```

## NOTES:
- `restart` is required to be called when `stop`-ed and `run` is about to be called.
- due to the nature of UDP, if one side gets "destroyed" and is able to reconnect "immediately", disconnection will not be "detected".
- (will be fixed in the future) - host is not resolved. currently expects only IP.