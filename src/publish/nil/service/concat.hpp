#pragma once

#include "codec.hpp"

#include <array>
#include <vector>

namespace nil::service
{
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
        const std::array<std::vector<std::uint8_t>, sizeof...(T)> buffers
            = {codec<T>::serialize(data)...};

        std::vector<std::uint8_t> message;
        message.reserve(
            [&]()
            {
                auto c = 0ul;
                for (const auto& b : buffers)
                {
                    c += b.size();
                }
                return c;
            }()
        );
        for (const auto& buffer : buffers)
        {
            for (const auto& item : buffer)
            {
                message.push_back(item);
            }
        }
        return message;
    }
}
