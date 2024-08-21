#pragma once

#include "../ID.hpp"
#include "AutoCast.hpp"
#include "Callable.hpp"
#include "errors.hpp"

#include <memory>
#include <type_traits>
#include <utility>

namespace nil::service::detail
{
    /**
     * @brief This is used to detect if the user provided a callable with `const auto&` argument.
     *  If const auto& is provided, the AutoCast passed by the library will not trigger implicit
     * conversion. This means that in the body of the callable, data is not yet consumed which will
     * cause weird behavior.
     */
    struct Unknown
    {
    };

    using icallable_t = ICallable<const ID&, const void*, std::uint64_t>;
    template <typename T>
    using callable_t = Callable<T, const ID&, const void*, std::uint64_t>;

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
     * @return std::unique_ptr<ICallable<const ID&, const void*, std::uint64_t>>
     */

    template <typename Handler>
    std::unique_ptr<icallable_t> create_message_handler(Handler handler)
    {
        (void)handler;
        argument_error<Handler>("unsupported handler type");
        return {};
    }

    template <typename Handler>
        requires(std::is_invocable_v<Handler, const ID&, const void*, std::uint64_t>)
    std::unique_ptr<icallable_t> create_message_handler(Handler handler)
    {
        using callable_t = Callable<Handler, const ID&, const void*, std::uint64_t>;
        return std::make_unique<callable_t>(std::move(handler));
    }

    template <typename Handler>
        requires(std::is_invocable_v<Handler>)
    std::unique_ptr<icallable_t> create_message_handler(Handler handler)
    {
        return create_message_handler(              //
            [handler = std::move(handler)]          //
            (const ID&, const void*, std::uint64_t) //
            { handler(); }
        );
    }

    template <typename Handler>
        requires(std::is_invocable_v<Handler, const ID&>)
    std::unique_ptr<icallable_t> create_message_handler(Handler handler)
    {
        return create_message_handler(                 //
            [handler = std::move(handler)]             //
            (const ID& id, const void*, std::uint64_t) //
            { handler(id); }
        );
    }

    template <typename Handler>
        requires(
            !std::is_invocable_v<Handler, const ID&, const Unknown&>
            && std::is_invocable_v<Handler, const ID&, const AutoCast&> //
        )
    std::unique_ptr<icallable_t> create_message_handler(Handler handler)
    {
        return create_message_handler(                           //
            [handler = std::move(handler)]                       //
            (const ID& id, const void* data, std::uint64_t size) //
            { handler(id, AutoCast(data, &size)); }
        );
    }

    template <typename Handler>
        requires(
            !std::is_invocable_v<Handler, const ID&, std::uint64_t>
            && std::is_invocable_v<Handler, const void*, std::uint64_t> //
        )
    std::unique_ptr<icallable_t> create_message_handler(Handler handler)
    {
        return create_message_handler(                        //
            [handler = std::move(handler)]                    //
            (const ID&, const void* data, std::uint64_t size) //
            { handler(data, size); }
        );
    }

    template <typename Handler>
        requires(
            !std::is_invocable_v<Handler, const ID&>
            && !std::is_invocable_v<Handler, const Unknown&>
            && std::is_invocable_v<Handler, const AutoCast&> //
        )
    std::unique_ptr<icallable_t> create_message_handler(Handler handler)
    {
        return create_message_handler(                        //
            [handler = std::move(handler)]                    //
            (const ID&, const void* data, std::uint64_t size) //
            { handler(AutoCast{data, &size}); }
        );
    }
}
