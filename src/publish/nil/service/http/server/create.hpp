#pragma once

#include "../../structs.hpp"

#include <cstdint>

namespace nil::service::http::server
{
    struct Options final
    {
        std::string host;
        std::uint16_t port = 0;
        std::uint64_t buffer = 8192;
    };

    W create(Options options);
}
