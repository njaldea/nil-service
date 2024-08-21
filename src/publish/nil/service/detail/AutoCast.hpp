#pragma once

#include "../ID.hpp"
#include "../codec.hpp"

namespace nil::service::detail
{
    /**
     * @brief a hack to bypass argument type detection of the handlers.
     *  this is only used internally to avoid convoluted templates just to detect the type.
     */
    struct AutoCast final
    {
        AutoCast(const void* init_d, std::uint64_t* init_s)
            : d(init_d)
            , s(init_s)
        {
        }

        ~AutoCast() = default;

        AutoCast(const AutoCast&) = delete;
        AutoCast(AutoCast&&) = delete;
        AutoCast& operator=(const AutoCast&) = delete;
        AutoCast& operator=(AutoCast&&) = delete;

        template <typename T>
        operator T() const // NOLINT
        {
            return codec<T>::deserialize(d, *s);
        }

        operator ID() const = delete;

        const void* d;
        std::uint64_t* s;
    };
}
