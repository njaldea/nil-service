#include <nil/service/http/Server.hpp>

#include "WebSocketImpl.hpp"

namespace nil::service::http
{
    WebSocket::WebSocket(std::unique_ptr<Impl> init_impl)
        : impl(std::move(init_impl))
    {
        impl->parent = this;
    }

    WebSocket::~WebSocket() noexcept = default;

    void WebSocket::run()
    {
    }

    void WebSocket::stop()
    {
    }

    void WebSocket::restart()
    {
    }

    void WebSocket::publish(std::vector<std::uint8_t> data)
    {
        if (impl->context != nullptr)
        {
            boost::asio::post(
                *impl->context,
                [this, msg = std::move(data)]()
                {
                    for (const auto& item : impl->connections)
                    {
                        item.second->write(msg.data(), msg.size());
                    }
                }
            );
        }
    }

    void WebSocket::send(const ID& id, std::vector<std::uint8_t> data)
    {
        if (impl->context != nullptr)
        {
            boost::asio::post(
                *impl->context,
                [this, id, msg = std::move(data)]()
                {
                    const auto it = impl->connections.find(id);
                    if (it != impl->connections.end())
                    {
                        it->second->write(msg.data(), msg.size());
                    }
                }
            );
        }
    }
}
