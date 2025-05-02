#pragma once

#include "Callable.hpp"

#include <nil/xalt/errors.hpp>

#include <memory>
#include <type_traits>
#include <utility>

namespace nil::service
{
    struct ID;
}

namespace nil::service::detail
{
    /**
     * @brief adapter method so that the handler can be converted to appropriate type.
     *  Handler is expected to have the following signature:
     *   -  void method(const ID&)
     *   -  void method()
     *
     * @tparam Handler
     * @param handler
     * @return std::unique_ptr<ICallable<const ID&>>
     */
    template <typename Handler>
    std::unique_ptr<ICallable<const ID&>> create_handler(Handler handler)
    {
        constexpr auto match                          //
            = std::is_invocable_v<Handler, const ID&> //
            + std::is_invocable_v<Handler>;
        if constexpr (0 == match) // NOLINTNEXTLINE(bugprone-branch-clone)
        {
            nil::xalt::undefined<Handler>(); // incorrect argument type detected
        }
        else if constexpr (1 < match)
        {
            nil::xalt::undefined<Handler>(); // ambiguous argument type detected
        }
        else if constexpr (std::is_invocable_v<Handler, const ID&>)
        {
            using callable_t = detail::Callable<Handler, const ID&>;
            return std::make_unique<callable_t>(std::move(handler));
        }
        else if constexpr (std::is_invocable_v<Handler>)
        {
            return create_handler([handler = std::move(handler)](const ID&) { handler(); });
        }
        else
        {
            nil::xalt::undefined<Handler>(); // incorrect argument type detected
        }
    }
}
