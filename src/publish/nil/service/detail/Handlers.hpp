#pragma once

#include "../ID.hpp"
#include "Callable.hpp"

#include <memory>

namespace nil::service::detail
{
    struct Handlers
    {
        std::unique_ptr<ICallable<const ID&, const void*, std::uint64_t>> msg;
        std::unique_ptr<ICallable<const ID&>> ready;
        std::unique_ptr<ICallable<const ID&>> connect;
        std::unique_ptr<ICallable<const ID&>> disconnect;
    };
}
