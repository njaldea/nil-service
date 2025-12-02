#pragma once

#include "../../structs.hpp"

#include <cstdint>
#include <memory>
#include <string>

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

    std::unique_ptr<IStandaloneService> create(Options options);
}
