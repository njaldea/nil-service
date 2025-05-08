#pragma once

#include <nil/service/ID.hpp>

#include "WebService.hpp"

#include "../https/server/WebSocket.hpp"

namespace nil::service
{
    struct HTTPSTransaction;

    struct HTTPSService: WebService
    {
        HTTPSService() = default;
        ~HTTPSService() noexcept override = default;
        HTTPSService(const HTTPSService&) = delete;
        HTTPSService(HTTPSService&&) = delete;
        HTTPSService& operator=(const HTTPSService&) = delete;
        HTTPSService& operator=(HTTPSService&&) = delete;

        Service* get_wss(const std::string& key) override
        {
            return &wss[key];
        }

        std::unordered_map<std::string, https::server::WebSocket> wss;
    };
}
