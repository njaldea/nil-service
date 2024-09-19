#pragma once

#include "RunnableService.hpp"
#include "Service.hpp"

namespace nil::service
{
    struct StandaloneService
        : Service
        , RunnableService
    {
        StandaloneService() = default;
        ~StandaloneService() noexcept override = default;
        StandaloneService(const StandaloneService&) = delete;
        StandaloneService(StandaloneService&&) = delete;
        StandaloneService& operator=(const StandaloneService&) = delete;
        StandaloneService& operator=(StandaloneService&&) = delete;
    };
}
