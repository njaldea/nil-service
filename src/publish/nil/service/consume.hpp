#pragma once

#include "detail.hpp"

#include <cstdint>

namespace nil::service
{
    /**
     * @brief utility method to cast the payload into a specific type.
     *  expect data and size to move.
     *
     * @tparam T
     * @param data
     * @param size
     * @return T
     */
    template <typename T>
    inline T consume(const void*& data, std::uint64_t& size)
    {
        const auto o_size = size;
        T casted = detail::AutoCast(data, &size);
        data = static_cast<const std::uint8_t*>(data) + o_size - size;
        return casted;
    }
}
