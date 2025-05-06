#pragma once

#ifdef NIL_SERVICE_SECURE

#include <nil/service/ID.hpp>

#include "RunnableService.hpp"

#include "../https/server/WebSocket.hpp"

#include <functional>
#include <vector>

namespace nil::service
{
    struct HTTPSTransaction;

    struct HTTPSService: RunnableService
    {
        HTTPSService() = default;
        ~HTTPSService() noexcept override = default;
        HTTPSService(const HTTPSService&) = delete;
        HTTPSService(HTTPSService&&) = delete;
        HTTPSService& operator=(const HTTPSService&) = delete;
        HTTPSService& operator=(HTTPSService&&) = delete;

        std::vector<std::function<void(const ID&)>> on_ready;
        std::function<void(const HTTPSTransaction&)> on_get;
        std::unordered_map<std::string, https::server::WebSocket> wss;
    };
}

#endif
