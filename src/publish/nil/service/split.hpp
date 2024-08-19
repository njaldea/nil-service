#pragma once

#include "ID.hpp"
#include "consume.hpp"
#include "detail.hpp"

#include <type_traits>

namespace nil::service
{
    /**
     * @brief splits the payload into two parts
     *  first part is T
     *  second part is dependent on the handler's last data type
     *      it can be the following:
     *       -  a type with codec
     *       -  const void* + std::uint64_t
     *
     * @tparam T
     * @tparam H
     * @param handler
     * @return auto
     */
    template <typename T, typename H>
    auto split(H handler)
    {
        return [handler = std::move(handler)](const ID& id, const void* data, std::uint64_t size)
        {
            using AutoCast = typename detail::AutoCast;
            using Unknown = typename detail::Unknown;

            constexpr auto match //
                = std::is_invocable_v<H, const ID&, const T&, const AutoCast&>
                + std::is_invocable_v<H, const ID&, const T&, const void*, std::uint64_t>
                + std::is_invocable_v<H, const T&, const AutoCast&>
                + std::is_invocable_v<H, const T&, const void*, std::uint64_t>;

            constexpr auto has_unknown_type //
                = std::is_invocable_v<H, const T&, const Unknown&>
                + std::is_invocable_v<H, const ID&, const T&, const Unknown&>;

            const auto value = consume<T>(data, size);

            if constexpr (0 == match)
            {
                detail::argument_error<H>("incorrect argument type detected");
            }
            else if constexpr (1 < match)
            {
                detail::argument_error<H>("ambiguous argument type detected");
            }
            else if constexpr (0 < has_unknown_type)
            {
                detail::argument_error<H>("unknown argument type detected");
            }
            else if constexpr (std::is_invocable_v<H, const ID&, const T&, const AutoCast&>)
            {
                handler(id, value, AutoCast(data, &size));
            }
            else if constexpr ( //
                std::is_invocable_v<
                    H,
                    const ID&,
                    const T&,
                    const void*,
                    std::uint64_t> //
            )
            {
                handler(id, value, data, size);
            }
            else if constexpr (std::is_invocable_v<H, const T&, const AutoCast&>)
            {
                handler(value, AutoCast(data, &size));
            }
            else if constexpr (std::is_invocable_v<H, const T&, const void*, std::uint64_t>)
            {
                handler(value, data, size);
            }
            else
            {
                detail::unreachable<H>();
            }
        };
    }
}
