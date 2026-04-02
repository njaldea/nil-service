#pragma once

#include "../ID.hpp"

#include <nil/xalt/errors.hpp>

#include <type_traits>
#include <utility>

namespace nil::service::detail
{
    /**
     * @brief adapter method so that the handler can be converted to appropriate type.
     *  Handler is expected to have the following signature:
     *   -  void method(ID)
     *   -  void method()
     *
     * @tparam Handler
     * @param handler
     * @return std::function<void(ID)>
     */
    template <typename Handler>
    auto create_handler(Handler handler)
    {
        if constexpr (std::is_invocable_v<Handler>)
        {
            return [handler = std::move(handler)](ID) { handler(); };
        }
        else if constexpr (std::is_invocable_v<Handler, ID>)
        {
            return [handler = std::move(handler)](ID id) { handler(id); };
        }
        else
        {
            nil::xalt::undefined<Handler>(); // handler type is not supported
        }
    }
}
