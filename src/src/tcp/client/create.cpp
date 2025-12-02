#include <nil/service/tcp/client/create.hpp>

#include "../../utils.hpp"
#include "../Connection.hpp"

#define BOOST_ASIO_STANDALONE
#define BOOST_ASIO_NO_TYPEID
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>

namespace nil::service::tcp::client
{
    struct Context
    {
        explicit Context()
            : strand(make_strand(ctx))
            , reconnection(strand)
        {
        }

        boost::asio::io_context ctx;
        boost::asio::strand<boost::asio::io_context::executor_type> strand;
        boost::asio::steady_timer reconnection;
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

        void publish_ex(ID id, std::vector<std::uint8_t> data) override
        {
            boost::asio::post(
                context->strand,
                [this, id = std::move(id), msg = std::move(data)]()
                {
                    if (connection != nullptr && connection->id() != id)
                    {
                        connection->write(msg.data(), msg.size());
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
                    if (connection != nullptr && connection->id() == id)
                    {
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
        Options options;
        std::unique_ptr<Context> context;
        std::unique_ptr<Connection> connection;

        std::vector<std::function<void(const ID&, const void*, std::uint64_t)>> on_message_cb;
        std::vector<std::function<void(const ID&)>> on_ready_cb;
        std::vector<std::function<void(const ID&)>> on_connect_cb;
        std::vector<std::function<void(const ID&)>> on_disconnect_cb;

        void connect(Connection* target_connection) override
        {
            utils::invoke(on_connect_cb, target_connection->id());
        }

        void disconnect(Connection* target_connection) override
        {
            boost::asio::post(
                context->strand,
                [this, target_connection]()
                {
                    if (connection.get() == target_connection)
                    {
                        utils::invoke(on_disconnect_cb, connection->id());
                        connection.reset();
                    }
                    reconnect();
                }
            );
        }

        void message(const ID& id, const void* data, std::uint64_t size) override
        {
            utils::invoke(on_message_cb, id, data, size);
        }

        void connect()
        {
            auto socket = std::make_unique<boost::asio::ip::tcp::socket>(context->strand);
            auto* socket_ptr = socket.get();
            socket_ptr->async_connect(
                {boost::asio::ip::make_address(options.host.data()), options.port},
                [this, socket = std::move(socket)](const boost::system::error_code& ec)
                {
                    if (!ec)
                    {
                        utils::invoke(on_ready_cb, utils::to_id(socket->local_endpoint()));
                        connection = std::make_unique<Connection>(
                            options.buffer,
                            std::move(*socket),
                            *this
                        );
                        return;
                    }
                    reconnect();
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
