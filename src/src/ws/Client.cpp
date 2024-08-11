#include <nil/service/ws/Client.hpp>

#include "Connection.hpp"
#include "nil/service/IService.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>

namespace nil::service::ws
{
    struct Client::Impl final: IImpl
    {
        explicit Impl(const Options& init_options, const detail::Handlers& init_handlers)
            : options(init_options)
            , handlers(init_handlers)
            , strand(boost::asio::make_strand(context))
            , reconnection(strand)
        {
        }

        ~Impl() override = default;

        Impl(Impl&&) noexcept = delete;
        Impl& operator=(Impl&&) noexcept = delete;

        Impl(const Impl&) = delete;
        Impl& operator=(const Impl&) = delete;

        void send(const ID& id, std::vector<std::uint8_t> data)
        {
            boost::asio::dispatch(
                strand,
                [this, id, msg = std::move(data)]()
                {
                    if (connection != nullptr && connection->id() == id)
                    {
                        connection->write(msg.data(), msg.size());
                    }
                }
            );
        }

        void publish(std::vector<std::uint8_t> data)
        {
            boost::asio::dispatch(
                strand,
                [this, msg = std::move(data)]()
                {
                    if (connection != nullptr)
                    {
                        connection->write(msg.data(), msg.size());
                    }
                }
            );
        }

        void disconnect(Connection* target_connection) override
        {
            boost::asio::dispatch(
                strand,
                [this, target_connection]()
                {
                    if (connection.get() == target_connection)
                    {
                        if (handlers.disconnect)
                        {
                            handlers.disconnect->call(connection->id());
                        }
                        connection.reset();
                    }
                    reconnect();
                }
            );
        }

        void message(const ID& id, const std::uint8_t* data, std::uint64_t size) override
        {
            if (handlers.msg)
            {
                handlers.msg->call(id, data, size);
            }
        }

        void connect()
        {
            auto socket = std::make_unique<boost::asio::ip::tcp::socket>(strand);
            auto* socket_ptr = socket.get();
            socket_ptr->async_connect(
                {boost::asio::ip::make_address(options.host.data()), options.port},
                [this, socket = std::move(socket)](const boost::system::error_code& connect_ec)
                {
                    if (connect_ec)
                    {
                        reconnect();
                        return;
                    }
                    namespace websocket = boost::beast::websocket;
                    auto ws = std::make_unique<websocket::stream<boost::beast::tcp_stream>>(
                        std::move(*socket)
                    );
                    ws->set_option(
                        websocket::stream_base::timeout::suggested(boost::beast::role_type::client)
                    );
                    ws->set_option(websocket::stream_base::decorator(
                        [](websocket::request_type& req)
                        {
                            req.set(
                                boost::beast::http::field::user_agent,
                                std::string(BOOST_BEAST_VERSION_STRING) + " websocket-client-async"
                            );
                        }
                    ));
                    auto* ws_ptr = ws.get();
                    ws_ptr->async_handshake(
                        options.host + ':' + std::to_string(options.port),
                        "/",
                        [this, ws = std::move(ws)](boost::beast::error_code ec)
                        {
                            if (ec)
                            {
                                reconnect();
                                return;
                            }
                            connection = std::make_unique<Connection>(
                                options.buffer,
                                std::move(*ws),
                                *this
                            );

                            if (handlers.connect)
                            {
                                handlers.connect->call(connection->id());
                            }
                        }
                    );
                }
            );
        }

        void reconnect()
        {
            reconnection.expires_after(std::chrono::milliseconds(25));
            reconnection.async_wait(
                [this](const boost::system::error_code& ec)
                {
                    if (ec != boost::asio::error::operation_aborted)
                    {
                        connect();
                    }
                }
            );
        }

        const Options& options;
        const detail::Handlers& handlers;

        boost::asio::io_context context;
        boost::asio::strand<boost::asio::io_context::executor_type> strand;
        boost::asio::steady_timer reconnection;
        std::unique_ptr<Connection> connection;
    };

    Client::Client(Client::Options init_options)
        : options{std::move(init_options)}
        , impl(std::make_unique<Impl>(options, handlers))
    {
        impl->connect();
    }

    Client::~Client() noexcept = default;

    void Client::run()
    {
        impl->context.run();
    }

    void Client::stop()
    {
        impl->context.stop();
    }

    void Client::restart()
    {
        impl.reset();
        impl = std::make_unique<Impl>(options, handlers);
        impl->connect();
    }

    void Client::send(const ID& id, std::vector<std::uint8_t> data)
    {
        impl->send(id, std::move(data));
    }

    void Client::publish(std::vector<std::uint8_t> data)
    {
        impl->publish(std::move(data));
    }
}
