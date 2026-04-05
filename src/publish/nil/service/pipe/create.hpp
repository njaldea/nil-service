#pragma once

#if defined(__unix__) || defined(__unix) || defined(unix) || defined(__APPLE__)

#include "../structs.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <string_view>

namespace nil::service::pipe
{
    struct Options final
    {
        // Return semantics:
        // make_read: -1 disabled, -2 retry later, >=0 owned read fd.
        // make_write: -1 disabled, >=0 owned write fd (no retry path).
        std::function<int()> make_read;
        std::function<int()> make_write;
        /**
         * @brief buffer size to use:
         *  - maximum payload size delivered while receiving
         *  - larger payloads disconnect the current endpoints before reconnect
         */
        std::uint64_t buffer = 1024;
    };

    std::unique_ptr<IStandaloneService> create(Options options);

    int mkfifo(std::string_view path);
    int r_mkfifo(std::string_view path);
    int w_mkfifo(std::string_view path);
}

#define NIL_SERVICE_PIPE_SUPPORTED 1
#else
#define NIL_SERVICE_PIPE_SUPPORTED 0
#endif
