#include <nil/service/ws/Server.hpp>

#include "../utils.hpp"
#include "Connection.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>

namespace nil::service::ws
{
    struct Server::Impl final: ConnectedImpl<Connection>
    {
    public:
        explicit Impl(const Server& parent)
            : options(parent.options)
            , handlers(parent.handlers)
            , strand(boost::asio::make_strand(context))
            , endpoint(boost::asio::ip::make_address("0.0.0.0"), options.port)
            , acceptor(strand, endpoint, true)
        {
        }

        ~Impl() override = default;

        Impl(Impl&&) noexcept = delete;
        Impl& operator=(Impl&&) noexcept = delete;

        Impl(const Impl&) = delete;
        Impl& operator=(const Impl&) = delete;

        void run()
        {
            if (handlers.ready)
            {
                handlers.ready->call(utils::to_id(acceptor.local_endpoint()));
            }
            accept();
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
                    const auto it = connections.find(id);
                    if (it != connections.end())
                    {
                        it->second->write(msg.data(), msg.size());
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
                    for (const auto& item : connections)
                    {
                        item.second->write(msg.data(), msg.size());
                    }
                }
            );
        }

    private:
        void connect(ws::Connection* connection) override
        {
            if (handlers.connect)
            {
                handlers.connect->call(connection->id());
            }
        }

        void disconnect(Connection* connection) override
        {
            boost::asio::post(
                strand,
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

        void message(const ID& id, const void* data, std::uint64_t size) override
        {
            if (handlers.msg)
            {
                handlers.msg->call(id, data, size);
            }
        }

        void accept()
        {
            acceptor.async_accept(
                context,
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

        const Options& options;
        const detail::Handlers& handlers;

        boost::asio::io_context context;
        boost::asio::strand<boost::asio::io_context::executor_type> strand;
        boost::asio::ip::tcp::endpoint endpoint;
        boost::asio::ip::tcp::acceptor acceptor;
        std::unordered_map<ID, std::unique_ptr<Connection>> connections;
    };

    Server::Server(Server::Options init_options)
        : options{init_options}
    {
    }

    Server::~Server() noexcept = default;

    void Server::run()
    {
        impl = std::make_unique<Impl>(*this);
        impl->run();
    }

    void Server::stop()
    {
        if (impl)
        {
            impl->stop();
        }
    }

    void Server::restart()
    {
        impl.reset();
    }

    void Server::send(const ID& id, std::vector<std::uint8_t> data)
    {
        if (impl)
        {
            impl->send(id, std::move(data));
        }
    }

    void Server::publish(std::vector<std::uint8_t> data)
    {
        if (impl)
        {
            impl->publish(std::move(data));
        }
    }
}
