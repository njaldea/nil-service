#pragma once

#include <cstdint>
#include <functional>
#include <vector>

namespace nil::service
{
    struct ID;
}

namespace nil::service::detail
{
    struct Handlers
    {
        std::vector<std::function<void(const ID&, const void*, std::uint64_t)>> on_message;
        std::vector<std::function<void(const ID&)>> on_ready;
        std::vector<std::function<void(const ID&)>> on_connect;
        std::vector<std::function<void(const ID&)>> on_disconnect;
    };

    template <typename... T>
    void invoke(auto& invocables, const T&... args)
    {
        for (const auto& invocable : invocables)
        {
            invocable(args...);
        }
    }
}
