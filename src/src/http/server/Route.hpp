#pragma once

#include <nil/service/detail/Callable.hpp>

#include <memory>
#include <string>

namespace nil::service::http::server
{
    struct Route
    {
        std::string content_type;
        std::unique_ptr<detail::ICallable<std::ostream&>> body;
    };
}
