#include <nil/service/tcp/server/create.hpp>

#include "../../utils.hpp"
#include "../Connection.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>

#include <algorithm>

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
        static std::string to_string_local(const void* c)
        {
            return utils::to_id(static_cast<const Impl*>(c)->context->acceptor.local_endpoint());
        }

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
                utils::invoke(on_ready_cb, ID{this, this, Impl::to_string_local});
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

        void dispatch(std::function<void()> task) override
        {
            if (context)
            {
                boost::asio::dispatch(context->ctx, std::move(task));
            }
        }

        void publish(std::vector<std::uint8_t> data) override
        {
            boost::asio::post(
                context->strand,
                [this, msg = std::move(data)]()
                {
                    for (const auto& connection : connections)
                    {
                        connection->write(msg.data(), msg.size());
                    }
                }
            );
        }

        void publish_ex(std::vector<ID> ids, std::vector<std::uint8_t> data) override
        {
            boost::asio::post(
                context->strand,
                [this, ids = std::move(ids), msg = std::move(data)]()
                {
                    for (const auto& connection : connections)
                    {
                        if (ids.end() == std::find(ids.begin(), ids.end(), connection->remote_id()))
                        {
                            continue;
                        }

                        connection->write(msg.data(), msg.size());
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
                        auto it = std::find_if(
                            connections.begin(),
                            connections.end(),
                            [&id](const auto& connection) { return id == connection->remote_id(); }
                        );
                        if (it != connections.end())
                        {
                            (*it)->write(msg.data(), msg.size());
                        }
                    }
                }
            );
        }

    private:
        Options options;
        std::unique_ptr<Context> context;
        std::vector<std::unique_ptr<Connection>> connections;

        std::vector<std::function<void(ID, const void*, std::uint64_t)>> on_message_cb;
        std::vector<std::function<void(ID)>> on_ready_cb;
        std::vector<std::function<void(ID)>> on_connect_cb;
        std::vector<std::function<void(ID)>> on_disconnect_cb;

        void connect(Connection* connection) override
        {
            utils::invoke(on_connect_cb, connection->remote_id());
        }

        void disconnect(Connection* connection) override
        {
            boost::asio::post(
                context->strand,
                [this, id = connection->remote_id()]()
                {
                    connections.erase(
                        std::remove_if(
                            connections.begin(),
                            connections.end(),
                            [&id](const auto& current) { return current->remote_id() == id; }
                        ),
                        connections.end()
                    );
                    utils::invoke(on_disconnect_cb, id);
                }
            );
        }

        void message(ID id, const void* data, std::uint64_t size) override
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
                        connection->start();
                        connections.push_back(std::move(connection));
                    }
                    accept();
                }
            );
        }

        void impl_on_message(std::function<void(ID, const void*, std::uint64_t)> handler) override
        {
            on_message_cb.push_back(std::move(handler));
        }

        void impl_on_ready(std::function<void(ID)> handler) override
        {
            on_ready_cb.push_back(std::move(handler));
        }

        void impl_on_connect(std::function<void(ID)> handler) override
        {
            on_connect_cb.push_back(std::move(handler));
        }

        void impl_on_disconnect(std::function<void(ID)> handler) override
        {
            on_disconnect_cb.push_back(std::move(handler));
        }
    };

    std::unique_ptr<IStandaloneService> create(Options options)
    {
        return std::make_unique<Impl>(std::move(options));
    }
}
