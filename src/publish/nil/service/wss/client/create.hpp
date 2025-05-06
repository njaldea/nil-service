#pragma once

#include "../../structs.hpp"

namespace nil::service::wss::client
{
    struct Options final
    {
        std::string host;
        std::uint16_t port;
        /**
         * @brief buffer size to use:
         *  - one for receiving
         */
        std::uint64_t buffer = 1024;
    };

    A create(Options options);
}
