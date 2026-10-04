// Copyright (c) 2026, Neil Aldea <njaldea@gmail.com>
// SPDX-License-Identifier: BSL-1.0
// See LICENSE and https://www.boost.org/LICENSE_1_0.txt.

#pragma once

#include "../../structs.hpp"

#include <cstdint>
#include <memory>

namespace nil::service::http::server
{
    struct Options final
    {
        std::string host;
        std::uint16_t port = 0;
        std::uint64_t buffer = 8192;
    };

    std::unique_ptr<IWebService> create(Options options);
}
