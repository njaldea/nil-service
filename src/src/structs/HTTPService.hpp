#pragma once

#include "RunnableService.hpp"

#include "../http/server/WebSocket.hpp"

#include <string>
#include <vector>

namespace nil::service
{
    struct HTTPTransaction;

    struct HTTPService: RunnableService
    {
        HTTPService() = default;
        ~HTTPService() noexcept override = default;
        HTTPService(const HTTPService&) = delete;
        HTTPService(HTTPService&&) = delete;
        HTTPService& operator=(const HTTPService&) = delete;
        HTTPService& operator=(HTTPService&&) = delete;

        std::vector<std::function<void(const ID&)>> on_ready;
        std::function<void(const HTTPTransaction&)> on_get;
        std::unordered_map<std::string, http::server::WebSocket> wss;
    };
}
