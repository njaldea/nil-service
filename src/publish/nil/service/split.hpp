#pragma once

#include "ID.hpp"
#include "codec.hpp"
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

            const auto* const m = static_cast<const std::uint8_t*>(data);
            const auto o_size = size;
            const auto value = codec<T>::deserialize(data, size);
            const void* n_data = m + o_size - size;
            if constexpr (std::is_invocable_v<H, const ID&, const T&, const void*, std::uint64_t>)
            {
                handler(id, value, n_data, size);
            }
            else if constexpr (std::is_invocable_v<H, const ID&, const T&, const AutoCast&>)
            {
                handler(id, value, AutoCast(m + o_size - size, &size));
            }
            else if constexpr (std::is_invocable_v<H, const T&, const void*, std::uint64_t>)
            {
                handler(value, n_data, size);
            }
            else if constexpr (std::is_invocable_v<H, const T&, const AutoCast&>)
            {
                handler(value, AutoCast(m + o_size - size, &size));
            }
            else
            {
                detail::unreachable<T>();
            }
        };
    }
}
