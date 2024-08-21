#pragma once

#include "consume.hpp"
#include "detail/create_message_handler.hpp"

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
        constexpr static auto handle
            = [](const auto& handler, const ID& i, const void* d, std::uint64_t s)
        {
            handler->call(i, d, s);
            return true;
        };
        return //
            [... tags = std::move(handlers.value),
             ... handlers = detail::create_message_handler(std::move(handlers.callable))] //
            (const ID& id, const void* data, std::uint64_t size)
        {
            const auto value = consume<T>(data, size);
            ((value == tags && handle(handlers, id, data, size)) || ...);
        };
    }
}
