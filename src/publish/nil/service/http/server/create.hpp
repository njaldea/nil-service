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
