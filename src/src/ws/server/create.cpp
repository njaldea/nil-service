#include <nil/service/ws/server/create.hpp>

#include "../../structs/StandaloneService.hpp"
#include "../../utils.hpp"
#include "../Connection.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>

namespace nil::service::ws::server
{
    struct Context
    {
        explicit Context(const std::string& host, std::uint16_t port)
            : strand(make_strand(ctx))
            , endpoint(boost::asio::ip::make_address(host), port)
            , acceptor(boost::asio::ip::tcp::acceptor(strand, endpoint, true))
        {
        }

        boost::asio::io_context ctx;
        boost::asio::strand<boost::asio::io_context::executor_type> strand;
        boost::asio::ip::tcp::endpoint endpoint;
        boost::asio::ip::tcp::acceptor acceptor;
    };

    struct Impl final
        : StandaloneService
        , ConnectedImpl<Connection>
    {
    public:
        explicit Impl(Options init_options)
            : options(std::move(init_options))
        {
        }

        ~Impl() override = default;

        Impl(Impl&&) noexcept = delete;
        Impl& operator=(Impl&&) noexcept = delete;
        Impl(const Impl&) = delete;
        Impl& operator=(const Impl&) = delete;

        void start() override
        {
            if (!context)
            {
                context = std::make_unique<Context>(options.host, options.port);
                detail::invoke(handlers.on_ready, utils::to_id(context->acceptor.local_endpoint()));
                accept();
            }
            context->ctx.run();
        }

        void stop() override
        {
            if (context)
            {
                context->ctx.stop();
            }
        }

        void restart() override
        {
            context.reset();
        }

        void publish(std::vector<std::uint8_t> data) override
        {
            boost::asio::post(
                context->strand,
                [this, msg = std::move(data)]()
                {
                    for (const auto& item : connections)
                    {
                        item.second->write(msg.data(), msg.size());
                    }
                }
            );
        }

        void publish_ex(const ID& id, std::vector<std::uint8_t> data) override
        {
            boost::asio::post(
                context->strand,
                [this, id, msg = std::move(data)]()
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

        void send(const ID& id, std::vector<std::uint8_t> data) override
        {
            boost::asio::post(
                context->strand,
                [this, id, msg = std::move(data)]()
                {
                    const auto it = connections.find(id);
                    if (it != connections.end())
                    {
                        it->second->write(msg.data(), msg.size());
                    }
                }
            );
        }

        void send(const std::vector<ID>& ids, std::vector<std::uint8_t> data) override
        {
            boost::asio::post(
                context->strand,
                [this, ids, msg = std::move(data)]()
                {
                    for (const auto& id : ids)
                    {
                        const auto it = connections.find(id);
                        if (it != connections.end())
                        {
                            it->second->write(msg.data(), msg.size());
                        }
                    }
                }
            );
        }

    private:
        void connect(ws::Connection* connection) override
        {
            detail::invoke(handlers.on_connect, connection->id());
        }

        void disconnect(Connection* connection) override
        {
            boost::asio::post(
                context->strand,
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

        void message(const ID& id, const void* data, std::uint64_t size) override
        {
            detail::invoke(handlers.on_message, id, data, size);
        }

        void accept()
        {
            context->acceptor.async_accept(
                context->ctx,
                [this](
                    const boost::system::error_code& acceptor_ec,
                    boost::asio::ip::tcp::socket socket
                )
                {
                    if (!acceptor_ec)
                    {
                        auto id = utils::to_id(socket.remote_endpoint());
                        auto ws = std::make_unique<
                            boost::beast::websocket::stream<boost::beast::tcp_stream>>(
                            std::move(socket)
                        );
                        ws->set_option(boost::beast::websocket::stream_base::timeout::suggested(
                            boost::beast::role_type::server
                        ));
                        ws->set_option(boost::beast::websocket::stream_base::decorator(
                            [](boost::beast::websocket::response_type& res)
                            {
                                res.set(
                                    boost::beast::http::field::server,
                                    std::string(BOOST_BEAST_VERSION_STRING)
                                        + " websocket-server-async"
                                );
                            }
                        ));
                        auto* ws_ptr = ws.get();
                        ws_ptr->async_accept(
                            [this,
                             id = std::move(id),
                             ws = std::move(ws)](boost::beast::error_code ec)
                            {
                                if (ec)
                                {
                                    return;
                                }
                                auto connection = std::make_unique<Connection>(
                                    id,
                                    options.buffer,
                                    std::move(*ws),
                                    *this
                                );
                                connections.emplace(id, std::move(connection));
                            }
                        );
                    }
                    accept();
                }
            );
        }

        Options options;
        std::unique_ptr<Context> context;
        std::unordered_map<ID, std::unique_ptr<Connection>> connections;
    };

    A create(Options options)
    {
        constexpr auto deleter = [](StandaloneService* obj) { //
            auto ptr = static_cast<Impl*>(obj);               // NOLINT
            std::default_delete<Impl>()(ptr);
        };
        return {{new Impl(std::move(options)), deleter}};
    }
}
