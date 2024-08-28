#include <nil/service/http/Server.hpp>

#include "../ws/Connection.hpp"

#include <boost/asio/strand.hpp>

namespace nil::service::http
{
    struct WebSocket::Impl: ConnectedImpl<ws::Connection>
    {
        WebSocket* parent = nullptr;
        boost::asio::io_context* context = nullptr;
        std::unordered_map<ID, std::unique_ptr<ws::Connection>> connections;

        void ready(const ID& id) const
        {
            if (parent->handlers.ready)
            {
                parent->handlers.ready->call(id);
            }
        }

        void connect(ws::Connection* connection) override
        {
            if (parent->handlers.connect)
            {
                parent->handlers.connect->call(connection->id());
            }
        }

        void message(const ID& id, const void* data, std::uint64_t size) override
        {
            if (parent->handlers.msg)
            {
                parent->handlers.msg->call(id, data, size);
            }
        }

        void disconnect(ws::Connection* connection) override
        {
            boost::asio::post(
                *context,
                [this, id = connection->id()]()
                {
                    if (connections.contains(id))
                    {
                        connections.erase(id);
                    }
                    if (parent->handlers.disconnect)
                    {
                        parent->handlers.disconnect->call(id);
                    }
                }
            );
        }
    };
}
