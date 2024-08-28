#include <nil/service/ws/Client.hpp>

#include "../utils.hpp"
#include "Connection.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>

namespace nil::service::ws
{
    struct Client::Impl final: ConnectedImpl<Connection>
    {
    public:
        explicit Impl(const Client& parent)
            : options(parent.options)
            , handlers(parent.handlers)
            , strand(boost::asio::make_strand(context))
            , reconnection(strand)
        {
        }

        ~Impl() override = default;

        Impl(Impl&&) noexcept = delete;
        Impl& operator=(Impl&&) noexcept = delete;

        Impl(const Impl&) = delete;
        Impl& operator=(const Impl&) = delete;

        void run()
        {
            connect();
            context.run();
        }

        void stop()
        {
            context.stop();
        }

        void send(const ID& id, std::vector<std::uint8_t> data)
        {
            boost::asio::post(
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
            boost::asio::post(
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

    private:
        void connect(ws::Connection* target_connection) override
        {
            if (handlers.connect)
            {
                handlers.connect->call(target_connection->id());
            }
        }

        void disconnect(Connection* target_connection) override
        {
            boost::asio::post(
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

        void message(const ID& id, const void* data, std::uint64_t size) override
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
                    if (handlers.ready)
                    {
                        handlers.ready->call(utils::to_id(socket->local_endpoint()));
                    }
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
                            auto id = utils::to_id(                 //
                                boost::beast::get_lowest_layer(*ws) //
                                    .socket()
                                    .remote_endpoint()
                            );
                            connection = std::make_unique<Connection>(
                                id,
                                options.buffer,
                                std::move(*ws),
                                *this
                            );
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
    {
    }

    Client::~Client() noexcept = default;

    void Client::run()
    {
        impl = std::make_unique<Impl>(*this);
        impl->run();
    }

    void Client::stop()
    {
        if (impl)
        {
            impl->stop();
        }
    }

    void Client::restart()
    {
        impl.reset();
    }

    void Client::send(const ID& id, std::vector<std::uint8_t> data)
    {
        if (impl)
        {
            impl->send(id, std::move(data));
        }
    }

    void Client::publish(std::vector<std::uint8_t> data)
    {
        if (impl)
        {
            impl->publish(std::move(data));
        }
    }
}
