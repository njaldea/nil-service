#include <nil/service/wss/client/create.hpp>

#include "../../structs/StandaloneService.hpp"
#include "../../utils.hpp"
#include "../Connection.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>

#include <boost/asio/ssl/context.hpp>

namespace nil::service::wss::client
{
    struct Context
    {
        explicit Context()
            : strand(make_strand(ctx))
            , reconnection(strand)
            , ssl_context(boost::asio::ssl::context::tlsv12_client)
        {
            ssl_context.set_default_verify_paths();
        }

        boost::asio::io_context ctx;
        boost::asio::strand<boost::asio::io_context::executor_type> strand;
        boost::asio::steady_timer reconnection;
        boost::asio::ssl::context ssl_context;
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
                context = std::make_unique<Context>();
                connect();
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
                    if (connection != nullptr)
                    {
                        connection->write(msg.data(), msg.size());
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
                    if (connection != nullptr && connection->id() != id)
                    {
                        connection->write(msg.data(), msg.size());
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
                    if (connection != nullptr && connection->id() == id)
                    {
                        connection->write(msg.data(), msg.size());
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
                    if (connection != nullptr)
                    {
                        const auto& id = connection->id();
                        auto it = std::find(ids.begin(), ids.end(), id);
                        if (it != ids.end())
                        {
                            connection->write(msg.data(), msg.size());
                        }
                    }
                }
            );
        }

    private:
        void connect(wss::Connection* target_connection) override
        {
            detail::invoke(handlers.on_connect, target_connection->id());
        }

        void disconnect(Connection* target_connection) override
        {
            boost::asio::post(
                context->strand,
                [this, target_connection]()
                {
                    if (connection.get() == target_connection)
                    {
                        detail::invoke(handlers.on_disconnect, connection->id());
                        connection.reset();
                    }
                    reconnect();
                }
            );
        }

        void message(const ID& id, const void* data, std::uint64_t size) override
        {
            detail::invoke(handlers.on_message, id, data, size);
        }

        void connect()
        {
            auto socket = std::make_unique<boost::asio::ip::tcp::socket>(context->strand);
            auto* socket_ptr = socket.get();
            socket_ptr->async_connect(
                {boost::asio::ip::make_address(options.host), options.port},
                [this, socket = std::move(socket)](const boost::system::error_code& connect_ec)
                {
                    if (connect_ec)
                    {
                        reconnect();
                        return;
                    }

                    auto stream
                        = std::make_unique<boost::asio::ssl::stream<boost::asio::ip::tcp::socket>>(
                            std::move(*socket),
                            context->ssl_context
                        );

                    stream->async_handshake(
                        boost::asio::ssl::stream_base::client,
                        [this, stream = std::move(stream)](const boost::system::error_code& ssl_ec)
                        {
                            if (ssl_ec)
                            {
                                reconnect();
                                return;
                            }

                            auto ws = std::make_unique<boost::beast::websocket::stream<
                                boost::asio::ssl::stream<boost::asio::ip::tcp::socket>>>(
                                std::move(*stream)
                            );

                            ws->set_option(boost::beast::websocket::stream_base::timeout::suggested(
                                boost::beast::role_type::client
                            ));
                            ws->set_option(boost::beast::websocket::stream_base::decorator(
                                [](boost::beast::websocket::request_type& req)
                                {
                                    req.set(
                                        boost::beast::http::field::user_agent,
                                        std::string(BOOST_BEAST_VERSION_STRING)
                                            + " websocket-client-async"
                                    );
                                }
                            ));

                            ws->async_handshake(
                                options.host + ':' + std::to_string(options.port),
                                options.route,
                                [this, ws = std::move(ws)](boost::beast::error_code ec)
                                {
                                    if (ec)
                                    {
                                        reconnect();
                                        return;
                                    }

                                    auto& s = ws->next_layer().next_layer();
                                    detail::invoke(
                                        handlers.on_ready,
                                        utils::to_id(s.local_endpoint())
                                    );
                                    connection = std::make_unique<Connection>(
                                        utils::to_id(s.remote_endpoint()),
                                        options.buffer,
                                        std::move(*ws),
                                        *this
                                    );
                                }
                            );
                        }
                    );
                }
            );
        }

        void reconnect()
        {
            context->reconnection.expires_after(std::chrono::milliseconds(25));
            context->reconnection.async_wait(
                [this](const boost::system::error_code& ec)
                {
                    if (ec != boost::asio::error::operation_aborted)
                    {
                        connect();
                    }
                }
            );
        }

        Options options;
        std::unique_ptr<Context> context;
        std::unique_ptr<Connection> connection;
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
