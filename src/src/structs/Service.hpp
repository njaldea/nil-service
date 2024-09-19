#pragma once

#include "MessagingService.hpp"
#include "ObservableService.hpp"

namespace nil::service
{
    struct Service
        : MessagingService
        , ObservableService
    {
        Service() = default;
        ~Service() noexcept override = default;
        Service(const Service&) = delete;
        Service(Service&&) = delete;
        Service& operator=(const Service&) = delete;
        Service& operator=(Service&&) = delete;
    };
}
