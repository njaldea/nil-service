#pragma once

#include "codec.hpp"

#include <cstdint>

namespace nil::service
{
    /**
     * @brief utility method to cast the payload into a specific type.
     *  expect data and size to move.
     *
     * @tparam T
     * @param data - buffer to use
     *             - this is expected to be adjusted to the next
     *               valid position based on the data consumed
     *               during deserialization
     * @param size - available buffer size
     *             - this is expected to be adjusted by the data
     *               consumed during deserialization
     * @return T
     */
    template <typename T>
    T consume(const void*& data, std::uint64_t& size)
    {
        auto casted = codec<T>::deserialize(data, size);
        const auto consumed_size = codec<T>::size(casted);
        size -= consumed_size;
        data = static_cast<const std::uint8_t*>(data) + consumed_size;
        return casted;
    }
}
