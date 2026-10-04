// Copyright (c) 2026, Neil Aldea <njaldea@gmail.com>
// SPDX-License-Identifier: BSL-1.0
// See LICENSE and https://www.boost.org/LICENSE_1_0.txt.

#pragma once

#include "../structs.hpp"

#include <memory>

namespace nil::service::gateway
{
    std::unique_ptr<IGatewayService> create();
}
