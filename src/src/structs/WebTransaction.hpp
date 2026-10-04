// Copyright (c) 2026, Neil Aldea <njaldea@gmail.com>
// SPDX-License-Identifier: BSL-1.0
// See LICENSE and https://www.boost.org/LICENSE_1_0.txt.

#pragma once

#include <nil/service/structs.hpp>

#include <boost/beast/http.hpp>

namespace nil::service
{
    struct WebTransaction
    {
        boost::beast::http::request<boost::beast::http::dynamic_body>& request;   // NOLINT
        boost::beast::http::response<boost::beast::http::dynamic_body>& response; // NOLINT
    };
}
