#pragma once

#include "../ID.hpp"
#include "../codec.hpp"

#include <nil/xalt/errors.hpp>
#include <nil/xalt/fn_sign.hpp>
#include <nil/xalt/tlist.hpp>

#include <type_traits>
#include <utility>

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

    /**
     * @brief adapter method so that the handler can be converted to appropriate type.
     *  Handler is expected to have the following signature:
     *   -  void method()
     *   -  void method(const ID&)
     *   -  void method(const ID&, const WithCodec&)
     *   -  void method(const ID&, const void*, std::uint64_t)
     *   -  void method(const WithCodec&)
     *   -  void method(const void*, std::uint64_t)
     *
     * @tparam Handler
     * @param handler
     * @return std::function<void(const ID&, const void*, std::uint64_t)>
     */

    template <typename Handler>
    auto create_message_handler(Handler handler)
    {
        if constexpr (std::is_invocable_v<Handler, const ID&, const void*, std::uint64_t>)
        {
            return handler;
        }
        // no arg
        else if constexpr (std::is_invocable_v<Handler>)
        {
            return [handler = std::move(handler)] //
                (const ID&, const void*, std::uint64_t) { handler(); };
        }
        // one arg
        else if constexpr (std::is_invocable_v<Handler, const ID&>)
        {
            return [handler = std::move(handler)] //
                (const ID& id, const void*, std::uint64_t) { handler(id); };
        }
        else if constexpr (std::is_invocable_v<Handler, AutoCast>)
        {
            return [handler = std::move(handler)] //
                (const ID&, const void* data, std::uint64_t size)
            { handler(AutoCast{data, &size}); };
        }
        // two args
        else if constexpr (std::is_invocable_v<Handler, const void*, std::uint64_t>)
        {
            return [handler = std::move(handler)] //
                (const ID&, const void* data, std::uint64_t size) { handler(data, size); };
        }
        else if constexpr (std::is_invocable_v<Handler, const ID&, AutoCast>)
        {
            return [handler = std::move(handler)] //
                (const ID& id, const void* data, std::uint64_t size)
            { handler(id, AutoCast(data, &size)); };
        }
        else
        {
            nil::xalt::undefined<Handler>(); // handler type is not supported
        }
    }
}
