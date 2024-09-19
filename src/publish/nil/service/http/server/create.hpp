#pragma once

#include "../../structs.hpp"

namespace nil::service::http::server
{
    struct Options final
    {
        std::uint16_t port = 0;
        std::uint64_t buffer = 8192;
    };

    H create(Options options);
}
