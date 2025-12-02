#pragma once

#include "../structs.hpp"

#include <memory>

namespace nil::service::self
{
    std::unique_ptr<IStandaloneService> create();
}
