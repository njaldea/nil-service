// Copyright (c) 2026, Neil Aldea <njaldea@gmail.com>
// SPDX-License-Identifier: BSL-1.0
// See LICENSE and https://www.boost.org/LICENSE_1_0.txt.

#include "WebTransaction.hpp"

#include <boost/beast/core/ostream.hpp>

namespace nil::service
{
    std::string_view get_route(const WebTransaction& transaction)
    {
        return transaction.request.target();
    }

    void set_content_type(WebTransaction& transaction, std::string_view type)
    {
        transaction.response.set(boost::beast::http::field::content_type, type);
    }

    void send(const WebTransaction& transaction, std::string_view body)
    {
        transaction.response.result(boost::beast::http::status::ok);
        boost::beast::ostream(transaction.response.body()) << body;
    }

    void send(const WebTransaction& transaction, const std::istream& body)
    {
        transaction.response.result(boost::beast::http::status::ok);
        boost::beast::ostream(transaction.response.body()) << body.rdbuf();
    }
}
