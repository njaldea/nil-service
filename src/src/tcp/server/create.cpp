#include <nil/service/tcp/server/create.hpp>

#include "../../utils.hpp"
#include "../Connection.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>

namespace nil::service::tcp::server
{
    struct Context
    {
        explicit Context(const std::string& host, std::uint16_t port)
            : strand(make_strand(ctx))
            , acceptor(strand, {boost::asio::ip::make_address(host), port}, true)
        {
            acceptor.set_option(boost::asio::socket_base::reuse_address(true));
        }

        boost::asio::io_context ctx;
        boost::asio::strand<boost::asio::io_context::executor_type> strand;
        boost::asio::ip::tcp::acceptor acceptor;
    };

    struct Impl final
        : IStandaloneService
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
                utils::invoke(on_ready_cb, utils::to_id(context->acceptor.local_endpoint()));
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

        void publish_ex(ID id, std::vector<std::uint8_t> data) override
        {
            boost::asio::post(
                context->strand,
                [this, id = std::move(id), msg = std::move(data)]()
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

        void send(ID id, std::vector<std::uint8_t> data) override
        {
            boost::asio::post(
                context->strand,
                [this, id = std::move(id), msg = std::move(data)]()
                {
                    const auto it = connections.find(id);
                    if (it != connections.end())
                    {
                        it->second->write(msg.data(), msg.size());
                    }
                }
            );
        }

        void send(std::vector<ID> ids, std::vector<std::uint8_t> data) override
        {
            boost::asio::post(
                context->strand,
                [this, ids = std::move(ids), msg = std::move(data)]()
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
        Options options;
        std::unique_ptr<Context> context;
        std::unordered_map<ID, std::unique_ptr<Connection>> connections;

        std::vector<std::function<void(const ID&, const void*, std::uint64_t)>> on_message_cb;
        std::vector<std::function<void(const ID&)>> on_ready_cb;
        std::vector<std::function<void(const ID&)>> on_connect_cb;
        std::vector<std::function<void(const ID&)>> on_disconnect_cb;

        void connect(Connection* connection) override
        {
            utils::invoke(on_connect_cb, connection->id());
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
                    utils::invoke(on_disconnect_cb, id);
                }
            );
        }

        void message(const ID& id, const void* data, std::uint64_t size) override
        {
            utils::invoke(on_message_cb, id, data, size);
        }

        void accept()
        {
            context->acceptor.async_accept(
                context->ctx,
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

        void impl_on_message(std::function<void(const ID&, const void*, std::uint64_t)> handler
        ) override
        {
            on_message_cb.push_back(std::move(handler));
        }

        void impl_on_ready(std::function<void(const ID&)> handler) override
        {
            on_ready_cb.push_back(std::move(handler));
        }

        void impl_on_connect(std::function<void(const ID&)> handler) override
        {
            on_connect_cb.push_back(std::move(handler));
        }

        void impl_on_disconnect(std::function<void(const ID&)> handler) override
        {
            on_disconnect_cb.push_back(std::move(handler));
        }
    };

    std::unique_ptr<IStandaloneService> create(Options options)
    {
        return std::make_unique<Impl>(std::move(options));
    }
}
