#pragma once

#include "../../structs.hpp"

#include <chrono>
#include <cstdint>

namespace nil::service::udp::server
{
    struct Options final
    {
        std::string host;

        std::uint16_t port = 0;
        /**
         * @brief buffer size to use:
         *  - one for receiving per connection
         */
        std::uint64_t buffer = 1024;
        /**
         * @brief time to wait until a "connection" is considered as disconnected
         */
        std::chrono::nanoseconds timeout = std::chrono::seconds(2);
    };

    A create(Options options);
}
