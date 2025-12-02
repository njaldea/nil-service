#pragma once

#include "../../structs.hpp"

#include <cstdint>
#include <memory>
#include <string>

namespace nil::service::tcp::server
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
    };

    std::unique_ptr<IStandaloneService> create(Options options);
}
