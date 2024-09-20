#pragma once

#include "consume.hpp"
#include "detail/create_message_handler.hpp"

#include <array>
#include <cstdint>
#include <memory>
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
        struct Map
        {
            T value;
            std::unique_ptr<detail::icallable_t> handler;
        };

        using internal_t = std::array<Map, sizeof...(Handlers)>;
        auto internal_handlers = internal_t{
            Map(std::move(handlers.value),
                detail::create_message_handler(std::move(handlers.callable)))...
        };
        return                                                 //
            [internal_handlers = std::move(internal_handlers)] //
            (const ID& id, const void* data, std::uint64_t size)
        {
            const auto value = consume<T>(data, size);
            for (const auto& [tag, handler] : internal_handlers)
            {
                if (value == tag)
                {
                    handler->call(id, data, size);
                    return;
                }
            }
        };
    }
}
