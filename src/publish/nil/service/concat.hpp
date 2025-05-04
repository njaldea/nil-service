#pragma once

#include "codec.hpp"

#include <cstdint>
#include <iterator>
#include <vector>

namespace nil::service
{
    /**
     * @brief concatenates all of the data into one buffer.
     *  leverages on `codec` to serialize each data.
     *
     * @tparam T
     * @param buffer
     * @return std::size_t consumed buffer
     */
    template <typename... T>
    std::size_t concat_into(void* buffer, const T&... data)
    {
        auto* original_ptr = static_cast<std::uint8_t*>(buffer);
        auto* moving_ptr = original_ptr;
        ((original_ptr += codec<T>::serialize(original_ptr, data)), ...);
        return std::size_t(std::distance(moving_ptr, original_ptr));
    }

    /**
     * @brief concatenates all of the data into one buffer.
     *  leverages on `codec` to serialize each data.
     *
     * @tparam T
     * @param data
     * @return std::vector<std::uint8_t>
     */
    template <typename... T>
    std::vector<std::uint8_t> concat(const T&... data)
    {
        std::vector<std::uint8_t> message;
        message.resize((0 + ... + codec<T>::size(data)));
        concat_into(message.data(), data...);
        return message;
    }

}
