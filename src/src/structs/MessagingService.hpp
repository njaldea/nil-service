#pragma once

#include <nil/service/ID.hpp>

#include <cstdint>
#include <vector>

namespace nil::service
{
    struct MessagingService
    {
        MessagingService() = default;
        virtual ~MessagingService() noexcept = default;
        MessagingService(const MessagingService&) = delete;
        MessagingService(MessagingService&&) = delete;
        MessagingService& operator=(const MessagingService&) = delete;
        MessagingService& operator=(MessagingService&&) = delete;

        virtual void publish(std::vector<std::uint8_t> payload) = 0;
        virtual void send(const ID& id, std::vector<std::uint8_t> payload) = 0;
        virtual void send(const std::vector<ID>& ids, std::vector<std::uint8_t> payload) = 0;
    };
}
