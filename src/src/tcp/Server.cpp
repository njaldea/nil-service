#include <nil/service/tcp/Server.hpp>

#include "Connection.hpp"
#include "nil/service/IService.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>

namespace nil::service::tcp
{
    struct Server::Impl final: IImpl
    {
        explicit Impl(const Options& init_options, const detail::Handlers& init_handlers)
            : options(init_options)
            , handlers(init_handlers)
            , strand(boost::asio::make_strand(context))
            , acceptor(strand, {boost::asio::ip::make_address("0.0.0.0"), options.port}, true)
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
            boost::asio::dispatch(
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

        void disconnect(Connection* connection) override
        {
            boost::asio::dispatch(
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

        void message(const ID& id, const std::uint8_t* data, std::uint64_t size) override
        {
            if (handlers.msg)
            {
                handlers.msg->call(id, data, size);
            }
        }

        void accept()
        {
            acceptor.async_accept(
                boost::asio::make_strand(context),
                [this](const boost::system::error_code& ec, boost::asio::ip::tcp::socket socket)
                {
                    if (!ec)
                    {
                        auto connection = std::make_unique<Connection>(
                            options.buffer,
                            std::move(socket),
                            *this
                        );
                        const auto& id = connection->id();
                        connections.emplace(id, std::move(connection));
                        if (handlers.connect)
                        {
                            handlers.connect->call(id);
                        }
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
        , impl(std::make_unique<Impl>(options, handlers))
    {
        impl->accept();
    }

    Server::~Server() noexcept = default;

    void Server::run()
    {
        impl->context.run();
    }

    void Server::stop()
    {
        impl->context.stop();
    }

    void Server::restart()
    {
        impl.reset();
        impl = std::make_unique<Impl>(options, handlers);
        impl->accept();
    }

    void Server::send(const ID& id, std::vector<std::uint8_t> data)
    {
        impl->send(id, std::move(data));
    }

    void Server::publish(std::vector<std::uint8_t> data)
    {
        impl->publish(std::move(data));
    }
}
