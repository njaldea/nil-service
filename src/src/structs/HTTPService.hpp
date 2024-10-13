#pragma once

#include "RunnableService.hpp"

#include "../http/server/WebSocket.hpp"

#include <nil/service/detail/Callable.hpp>

#include <memory>
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

        std::vector<std::unique_ptr<detail::ICallable<const ID&>>> on_ready;
        std::unique_ptr<detail::ICallable<const HTTPTransaction&>> on_get;
        std::unordered_map<std::string, http::server::WebSocket> wss;
    };
}
