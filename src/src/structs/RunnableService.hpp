#pragma once

#include <nil/service/detail/Callable.hpp>

#include <memory>

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
        virtual void exec(std::unique_ptr<detail::ICallable<>> executable) = 0;
    };
}
