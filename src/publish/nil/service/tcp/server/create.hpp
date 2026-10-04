// Copyright (c) 2026, Neil Aldea <njaldea@gmail.com>
// SPDX-License-Identifier: BSL-1.0
// See LICENSE and https://www.boost.org/LICENSE_1_0.txt.

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
         *  - maximum payload size accepted while receiving per connection
         */
        std::uint64_t buffer = 1024;
    };

    std::unique_ptr<IStandaloneService> create(Options options);
}
