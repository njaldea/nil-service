#pragma once

#include <nil/service/detail/Handlers.hpp>

namespace nil::service
{
    struct ObservableService
    {
        ObservableService() = default;
        virtual ~ObservableService() noexcept = default;
        ObservableService(const ObservableService&) = delete;
        ObservableService(ObservableService&&) = delete;
        ObservableService& operator=(const ObservableService&) = delete;
        ObservableService& operator=(ObservableService&&) = delete;

        detail::Handlers handlers;
    };
}
