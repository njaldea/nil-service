#pragma once

#include <cstdint>

namespace nil::service
{
    struct ID;

    template <typename Connection>
    struct ConnectedImpl
    {
        ConnectedImpl() = default;
        virtual ~ConnectedImpl() noexcept = default;

        ConnectedImpl(ConnectedImpl&&) noexcept = delete;
        ConnectedImpl& operator=(ConnectedImpl&&) noexcept = delete;
        ConnectedImpl(const ConnectedImpl&) = delete;
        ConnectedImpl& operator=(const ConnectedImpl&) = delete;

        virtual void message(const ID& id, const void* data, std::uint64_t size) = 0;
        virtual void connect(Connection* connection) = 0;
        virtual void disconnect(Connection* connection) = 0;
    };
}
