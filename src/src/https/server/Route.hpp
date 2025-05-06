#pragma once

#include <functional>
#include <string>

namespace nil::service::https::server
{
    struct Route
    {
        std::string content_type;
        std::function<void(std::ostream&)> body;
    };
}
