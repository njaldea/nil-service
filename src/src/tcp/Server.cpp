#include <nil/service/tcp/Server.hpp>

#include "../utils.hpp"
#include "Connection.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>

namespace nil::service::tcp
{
    struct Server::Impl final: ConnectedImpl<Connection>
    {
    public:
        explicit Impl(const Server& parent)
            : options(parent.options)
            , handlers(parent.handlers)
            , strand(boost::asio::make_strand(context))
            , acceptor(strand, {boost::asio::ip::make_address("0.0.0.0"), options.port}, true)
        {
        }

        ~Impl() override = default;

        Impl(Impl&&) noexcept = delete;
        Impl& operator=(Impl&&) noexcept = delete;

        Impl(const Impl&) = delete;
        Impl& operator=(const Impl&) = delete;

        void ready()
        {
            if (handlers.ready)
            {
                handlers.ready->call(utils::to_id(acceptor.local_endpoint()));
            }
            accept();
        }

        void run()
        {
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
        void connect(Connection* connection) override
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
                [this](const boost::system::error_code& ec, boost::asio::ip::tcp::socket socket)
                {
                    if (!ec)
                    {
                        auto connection = std::make_unique<Connection>(
                            options.buffer,
                            std::move(socket),
                            *this
                        );
                        auto id = connection->id();
                        connections.emplace(std::move(id), std::move(connection));
                    }
                    accept();
                }
            );
        }

        const Options& options;
        const detail::Handlers& handlers;

        boost::asio::io_context context;
        boost::asio::strand<boost::asio::io_context::executor_type> strand;
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
        if (!impl)
        {
            impl = std::make_unique<Impl>(*this);
            impl->ready();
        }
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
