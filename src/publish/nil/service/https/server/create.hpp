#pragma once

#include "../../structs_ssl.hpp"

#include <cstdint>
#include <filesystem>

namespace nil::service::https::server
{
    struct Options final
    {
        std::filesystem::path cert;
        std::string host;
        std::uint16_t port = 0;
        std::uint64_t buffer = 8192;
    };

    S create(Options options);
}
