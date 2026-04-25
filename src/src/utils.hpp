#pragma once

#include <nil/service/ID.hpp>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/udp.hpp>

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>

namespace nil::service::utils
{
    template <typename Callables, typename... Args>
    void invoke(const Callables& cbs, const Args&... args)
    {
        for (const auto& cb : cbs)
        {
            if (cb)
            {
                cb(args...);
            }
        }
    }

    inline std::string to_id(const boost::asio::ip::tcp::endpoint& endpoint)
    {
        return {endpoint.address().to_string() + ":" + std::to_string(endpoint.port())};
    }

    inline std::string to_id(const boost::asio::ip::udp::endpoint& endpoint)
    {
        return {endpoint.address().to_string() + ":" + std::to_string(endpoint.port())};
    }

    // in tcp, we need to know the size of the actual message
    // to know until when to stop
    constexpr auto TCP_HEADER_SIZE = sizeof(std::uint64_t);

    // in udp, there is no connection guarantee.
    constexpr std::uint8_t UDP_INTERNAL_MESSAGE = 1u;
    constexpr std::uint8_t UDP_EXTERNAL_MESSAGE = 0u;

    constexpr auto PROBE_INTERVAL_MS = 25;

    constexpr auto TO_BITS = 8u;
    constexpr auto START_INDEX = 0u;

    template <typename T>
    std::array<std::uint8_t, sizeof(T)> to_array(T value)
    {
        std::array<std::uint8_t, sizeof(T)> retval{};
        if constexpr (std::is_same_v<T, bool>)
        {
            retval[0] = value ? 1 : 0;
        }
        else
        {
            std::memcpy(retval.data(), &value, sizeof(T));
            if constexpr (std::endian::native == std::endian::big)
            {
                std::reverse(retval.begin(), retval.end());
            }
        }
        return retval;
    }

    template <typename T>
    T from_array(const std::uint8_t* data)
    {
        if constexpr (std::is_same_v<T, bool>)
        {
            return data[0] != 0;
        }
        else
        {
            std::array<std::uint8_t, sizeof(T)> temp;
            std::memcpy(temp.data(), data, sizeof(T));
            if constexpr (std::endian::native == std::endian::big)
            {
                std::reverse(temp.begin(), temp.end());
            }
            T value;
            std::memcpy(&value, temp.data(), sizeof(T));
            return value;
        }
    }
}
