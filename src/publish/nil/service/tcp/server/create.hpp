#pragma once

#include "../../structs.hpp"

namespace nil::service::tcp::server
{
    struct Options final
    {
        std::uint16_t port = 0;
        /**
         * @brief buffer size to use:
         *  - one for receiving per connection
         */
        std::uint64_t buffer = 1024;
    };

    A create(Options options);
}
