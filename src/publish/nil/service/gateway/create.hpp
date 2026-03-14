#pragma once

#include "../structs.hpp"

#include <memory>

namespace nil::service::gateway
{
    std::unique_ptr<IGatewayService> create();
}
