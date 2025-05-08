#pragma once

#include "WebService.hpp"

#include "../http/server/WebSocket.hpp"

#include <string>

namespace nil::service
{
    struct WebTransaction;

    struct HTTPService: WebService
    {
        HTTPService() = default;
        ~HTTPService() noexcept override = default;
        HTTPService(const HTTPService&) = delete;
        HTTPService(HTTPService&&) = delete;
        HTTPService& operator=(const HTTPService&) = delete;
        HTTPService& operator=(HTTPService&&) = delete;

        Service* get_wss(const std::string& key) override
        {
            return &wss[key];
        }

        std::unordered_map<std::string, http::server::WebSocket> wss;
    };
}
