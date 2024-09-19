#pragma once

#include "RunnableService.hpp"

#include "../http/server/Route.hpp"
#include "../http/server/WebSocket.hpp"

namespace nil::service
{
    struct HTTPService: RunnableService
    {
        HTTPService() = default;
        ~HTTPService() noexcept override = default;
        HTTPService(const HTTPService&) = delete;
        HTTPService(HTTPService&&) = delete;
        HTTPService& operator=(const HTTPService&) = delete;
        HTTPService& operator=(HTTPService&&) = delete;

        std::vector<std::unique_ptr<detail::ICallable<const ID&>>> on_ready;
        std::unordered_map<std::string, http::server::WebSocket> wss;
        std::unordered_map<std::string, http::server::Route> routes;
    };
}
