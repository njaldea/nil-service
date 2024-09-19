#pragma once

#include <nil/service/structs.hpp>

namespace nil::service
{
    struct RunnableService
    {
        RunnableService() = default;
        virtual ~RunnableService() noexcept = default;
        RunnableService(const RunnableService&) = delete;
        RunnableService(RunnableService&&) = delete;
        RunnableService& operator=(const RunnableService&) = delete;
        RunnableService& operator=(RunnableService&&) = delete;

        virtual void start() = 0;
        virtual void stop() = 0;
        virtual void restart() = 0;
    };
}
