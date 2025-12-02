#include "WebSocket.hpp"

#include "../../utils.hpp"

namespace nil::service::https::server
{
    void WebSocket::ready(const ID& id) const
    {
        utils::invoke(on_ready_cb, id);
    }

    void WebSocket::connect(wss::Connection* connection)
    {
        utils::invoke(on_connect_cb, connection->id());
    }

    void WebSocket::message(const ID& id, const void* data, std::uint64_t size)
    {
        utils::invoke(on_message_cb, id, data, size);
    }

    void WebSocket::disconnect(wss::Connection* connection)
    {
        boost::asio::post(
            *context,
            [this, id = connection->id()]()
            {
                if (connections.contains(id))
                {
                    connections.erase(id);
                }
                utils::invoke(on_disconnect_cb, id);
            }
        );
    }

    void WebSocket::publish(std::vector<std::uint8_t> data)
    {
        if (context != nullptr)
        {
            boost::asio::post(
                *context,
                [this, msg = std::move(data)]()
                {
                    for (const auto& item : connections)
                    {
                        item.second->write(msg.data(), msg.size());
                    }
                }
            );
        }
    }

    void WebSocket::publish_ex(ID id, std::vector<std::uint8_t> data)
    {
        if (context != nullptr)
        {
            boost::asio::post(
                *context,
                [this, id = std::move(id), msg = std::move(data)]()
                {
                    for (const auto& item : connections)
                    {
                        if (item.first != id)
                        {
                            item.second->write(msg.data(), msg.size());
                        }
                    }
                }
            );
        }
    }

    void WebSocket::send(ID id, std::vector<std::uint8_t> data)
    {
        if (context != nullptr)
        {
            boost::asio::post(
                *context,
                [this, id = std::move(id), msg = std::move(data)]()
                {
                    auto it = connections.find(id);
                    if (it != connections.end())
                    {
                        it->second->write(msg.data(), msg.size());
                    }
                }
            );
        }
    }

    void WebSocket::send(std::vector<ID> ids, std::vector<std::uint8_t> data)
    {
        if (context != nullptr)
        {
            boost::asio::post(
                *context,
                [this, ids = std::move(ids), msg = std::move(data)]()
                {
                    for (const auto& id : ids)
                    {
                        auto it = connections.find(id);
                        if (it != connections.end())
                        {
                            it->second->write(msg.data(), msg.size());
                        }
                    }
                }
            );
        }
    }

    void WebSocket::impl_on_message(
        std::function<void(const ID&, const void*, std::uint64_t)> handler
    )
    {
        on_message_cb.push_back(std::move(handler));
    }

    void WebSocket::impl_on_ready(std::function<void(const ID&)> handler)
    {
        on_ready_cb.push_back(std::move(handler));
    }

    void WebSocket::impl_on_connect(std::function<void(const ID&)> handler)
    {
        on_connect_cb.push_back(std::move(handler));
    }

    void WebSocket::impl_on_disconnect(std::function<void(const ID&)> handler)
    {
        on_disconnect_cb.push_back(std::move(handler));
    }
}
