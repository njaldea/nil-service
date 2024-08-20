#pragma once

#include "consume.hpp"
#include "detail.hpp"

#include <cstdint>
#include <utility>

namespace nil::service
{
    template <typename T, typename Handler>
    struct Mapping
    {
        T value;
        Handler callable;
    };

    template <typename T, typename Handler>
    auto mapping(T value, Handler handler)
    {
        return Mapping{std::move(value), std::move(handler)};
    }

    template <typename T, typename... Handlers>
    auto map(Mapping<T, Handlers>... handlers)
    {
        return //
            [... tags = std::move(handlers.value),
             ... handlers = detail::create_message_handler(std::move(handlers.callable))] //
            (const nil::service::ID& id, const void* data, std::uint64_t size)
        {
            const auto value = nil::service::consume<T>(data, size);
            constexpr auto handle //
                = [](const auto& handler, const nil::service::ID& i, const void* d, std::uint64_t s)
            {
                handler->call(i, d, s);
                return true;
            };
            ((value == tags && handle(handlers, id, data, size)) || ...);
        };
    }
}
