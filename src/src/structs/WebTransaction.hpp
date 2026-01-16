#pragma once

#include <nil/service/structs.hpp>

#include <boost/beast/core/ostream.hpp>
#include <boost/beast/http.hpp>

namespace nil::service
{
    struct WebTransaction
    {
        boost::beast::http::request<boost::beast::http::dynamic_body>& request;   // NOLINT
        boost::beast::http::response<boost::beast::http::dynamic_body>& response; // NOLINT
    };
}
