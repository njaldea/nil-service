#pragma once

#include "../../structs.hpp"

namespace nil::service::ws::client
{
    struct Options final
    {
        std::string host;
        std::uint16_t port;
        std::string route = "/";
        /**
         * @brief buffer size to use:
         *  - one for receiving
         */
        std::uint64_t buffer = 1024;
    };

    A create(Options options);
}
