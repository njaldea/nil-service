#include "WebSocket.hpp"

namespace nil::service::http
{
    void WebSocket::ready(const ID& id) const
    {
        if (handlers.ready)
        {
            handlers.ready->call(id);
        }
    }

    void WebSocket::connect(ws::Connection* connection)
    {
        if (handlers.connect)
        {
            handlers.connect->call(connection->id());
        }
    }

    void WebSocket::message(const ID& id, const void* data, std::uint64_t size)
    {
        if (handlers.msg)
        {
            handlers.msg->call(id, data, size);
        }
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
                if (handlers.disconnect)
                {
                    handlers.disconnect->call(id);
                }
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

    void WebSocket::send(const ID& id, std::vector<std::uint8_t> data)
    {
        if (context != nullptr)
        {
            boost::asio::post(
                *context,
                [this, id, msg = std::move(data)]()
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
}
