#include "WebSocket.hpp"

#include "../../utils.hpp"

#include <algorithm>

namespace nil::service::http::server
{
    std::string WebSocket::to_string_local(const void* c)
    {
        return static_cast<const WebSocket*>(c)->route;
    }

    void WebSocket::set_route(std::string new_route)
    {
        route = std::move(new_route);
    }

    void WebSocket::ready()
    {
        utils::invoke(on_ready_cb, ID{this, this, WebSocket::to_string_local});
    }

    void WebSocket::connect(ws::Connection* connection)
    {
        utils::invoke(on_connect_cb, ID{this, connection, &ws::Connection::to_string_remote});
    }

    void WebSocket::message(const ID& id, const void* data, std::uint64_t size)
    {
        utils::invoke(on_message_cb, id, data, size);
    }

    void WebSocket::disconnect(ws::Connection* connection)
    {
        boost::asio::post(
            *context,
            [this, id = ID{this, connection, &ws::Connection::to_string_remote}]()
            {
                connections.erase(
                    std::remove_if(
                        connections.begin(),
                        connections.end(),
                        [&id](const auto& current) { return current->remote_id() == id; }
                    ),
                    connections.end()
                );
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
                    for (const auto& connection : connections)
                    {
                        connection->write(msg.data(), msg.size());
                    }
                }
            );
        }
    }

    void WebSocket::publish_ex(std::vector<ID> ids, std::vector<std::uint8_t> data)
    {
        if (context != nullptr)
        {
            boost::asio::post(
                *context,
                [this, ids = std::move(ids), msg = std::move(data)]()
                {
                    for (const auto& connection : connections)
                    {
                        if (ids.end() == std::find(ids.begin(), ids.end(), connection->remote_id()))
                        {
                            continue;
                        }

                        connection->write(msg.data(), msg.size());
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
                        auto it = std::find_if(
                            connections.begin(),
                            connections.end(),
                            [&id](const auto& connection) { return connection->remote_id() == id; }
                        );
                        if (it != connections.end())
                        {
                            (*it)->write(msg.data(), msg.size());
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
