#pragma once

#include <nil/service/structs.hpp>

#include "../../ConnectedImpl.hpp"
#include "../../ws/Connection.hpp"

namespace nil::service::http::server
{
    struct WebSocket final
        : public IService
        , public ConnectedImpl<ws::Connection>
    {
        friend struct Transaction;
        friend struct Impl;

    public:
        explicit WebSocket() = default;
        ~WebSocket() noexcept override = default;
        WebSocket(WebSocket&&) = delete;
        WebSocket(const WebSocket&) = delete;
        WebSocket& operator=(WebSocket&&) = delete;
        WebSocket& operator=(const WebSocket&) = delete;

        void publish(std::vector<std::uint8_t> data) override;
        void publish_ex(ID id, std::vector<std::uint8_t> data) override;
        void send(ID id, std::vector<std::uint8_t> data) override;
        void send(std::vector<ID> ids, std::vector<std::uint8_t> data) override;

        void ready(const ID& id) const;
        void connect(ws::Connection* connection) override;
        void message(const ID& id, const void* data, std::uint64_t size) override;
        void disconnect(ws::Connection* connection) override;

    private:
        std::unordered_map<ID, std::unique_ptr<ws::Connection>> connections;

        std::vector<std::function<void(const ID&, const void*, std::uint64_t)>> on_message_cb;
        std::vector<std::function<void(const ID&)>> on_ready_cb;
        std::vector<std::function<void(const ID&)>> on_connect_cb;
        std::vector<std::function<void(const ID&)>> on_disconnect_cb;

        void impl_on_message(std::function<void(const ID&, const void*, std::uint64_t)> handler
        ) override;
        void impl_on_ready(std::function<void(const ID&)> handler) override;
        void impl_on_connect(std::function<void(const ID&)> handler) override;
        void impl_on_disconnect(std::function<void(const ID&)> handler) override;

        boost::asio::io_context* context = nullptr;
    };
}
