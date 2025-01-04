#pragma once

#include "../../structs/Service.hpp"

#include "../../ConnectedImpl.hpp"
#include "../../ws/Connection.hpp"

namespace nil::service::http::server
{
    struct WebSocket final
        : public Service
        , public ConnectedImpl<ws::Connection>
    {
    public:
        explicit WebSocket() = default;
        ~WebSocket() noexcept override = default;
        WebSocket(WebSocket&&) = delete;
        WebSocket(const WebSocket&) = delete;
        WebSocket& operator=(WebSocket&&) = delete;
        WebSocket& operator=(const WebSocket&) = delete;

        void publish(std::vector<std::uint8_t> data) override;
        void publish_ex(const ID& id, std::vector<std::uint8_t> data) override;
        void send(const ID& id, std::vector<std::uint8_t> data) override;
        void send(const std::vector<ID>& id, std::vector<std::uint8_t> data) override;

        void ready(const ID& id) const;
        void connect(ws::Connection* connection) override;
        void message(const ID& id, const void* data, std::uint64_t size) override;
        void disconnect(ws::Connection* connection) override;

        boost::asio::io_context* context = nullptr;
        std::unordered_map<ID, std::unique_ptr<ws::Connection>> connections;
    };
}
