#pragma once

#include "RunnableService.hpp"
#include "Service.hpp"

#include <nil/service/ID.hpp>

#include <boost/beast/core/ostream.hpp>
#include <boost/beast/http.hpp>

namespace nil::service
{
    struct WebTransaction
    {
        boost::beast::http::request<boost::beast::http::dynamic_body>& request;   // NOLINT
        boost::beast::http::response<boost::beast::http::dynamic_body>& response; // NOLINT
    };

    struct WebService: RunnableService
    {
        WebService() = default;
        ~WebService() noexcept override = default;
        WebService(const WebService&) = delete;
        WebService(WebService&&) = delete;
        WebService& operator=(const WebService&) = delete;
        WebService& operator=(WebService&&) = delete;

        virtual Service* get_wss(const std::string& key) = 0;

        std::vector<std::function<void(const ID&)>> on_ready;
        std::function<void(const WebTransaction&)> on_get;
    };
}
