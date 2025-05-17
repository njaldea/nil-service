#include "WebSocket.hpp"

namespace nil::service::http::server
{
    void WebSocket::ready(const ID& id) const
    {
        detail::invoke(handlers.on_ready, id);
    }

    void WebSocket::connect(ws::Connection* connection)
    {
        detail::invoke(handlers.on_connect, connection->id());
    }

    void WebSocket::message(const ID& id, const void* data, std::uint64_t size)
    {
        detail::invoke(handlers.on_message, id, data, size);
    }

    void WebSocket::disconnect(ws::Connection* connection)
    {
        boost::asio::post(
            *context,
            [this, id = connection->id()]()
            {
                if (connections.contains(id))
                {
                    connections.erase(id);
                }
                detail::invoke(handlers.on_disconnect, id);
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
}
