#include "WebSocket.hpp"

#include "../../utils.hpp"

#include <algorithm>

namespace nil::service::http::server
{
    namespace
    {
        void write_payload(ws::Connection& connection, const std::vector<std::uint8_t>& msg)
        {
            connection.write(msg.data(), msg.size());
        }

        [[nodiscard]] bool contains_id(const std::vector<ID>& ids, const ID& id)
        {
            return ids.end() != std::find(ids.begin(), ids.end(), id);
        }

        [[nodiscard]] auto find_connection(
            std::vector<std::unique_ptr<ws::Connection>>& connections,
            const ID& id
        )
        {
            return std::find_if(
                connections.begin(),
                connections.end(),
                [&id](const auto& connection) { return connection->remote_id() == id; }
            );
        }
    }

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

    void WebSocket::message(ID id, const void* data, std::uint64_t size)
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
                        write_payload(*connection, msg);
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
                        if (!contains_id(ids, connection->remote_id()))
                        {
                            continue;
                        }

                        write_payload(*connection, msg);
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
                        auto it = find_connection(connections, id);
                        if (it != connections.end())
                        {
                            write_payload(**it, msg);
                        }
                    }
                }
            );
        }
    }

    void WebSocket::impl_on_message(std::function<void(ID, const void*, std::uint64_t)> handler)
    {
        on_message_cb.push_back(std::move(handler));
    }

    void WebSocket::impl_on_ready(std::function<void(ID)> handler)
    {
        on_ready_cb.push_back(std::move(handler));
    }

    void WebSocket::impl_on_connect(std::function<void(ID)> handler)
    {
        on_connect_cb.push_back(std::move(handler));
    }

    void WebSocket::impl_on_disconnect(std::function<void(ID)> handler)
    {
        on_disconnect_cb.push_back(std::move(handler));
    }
}
