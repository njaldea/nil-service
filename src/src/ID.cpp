// Copyright (c) 2026, Neil Aldea <njaldea@gmail.com>
// SPDX-License-Identifier: BSL-1.0
// See LICENSE and https://www.boost.org/LICENSE_1_0.txt.

#include <nil/service/ID.hpp>

namespace nil::service
{
    std::string to_string(ID id)
    {
        return id.to_string(id.id);
    }
}
