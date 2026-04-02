#include <nil/service/udp/server/create.hpp>

#include "../../utils.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>

#include <algorithm>

namespace nil::service::udp::server
{
    struct Context
    {
        explicit Context(const std::string& host, std::uint16_t port)
            : strand(make_strand(ctx))
            , socket(strand, {boost::asio::ip::make_address(host), port})
        {
            socket.set_option(boost::asio::socket_base::reuse_address(true));
        }

        boost::asio::io_context ctx;
        boost::asio::strand<boost::asio::io_context::executor_type> strand;
        boost::asio::ip::udp::socket socket;
    };

    struct Impl final: IStandaloneService
    {
        static std::string to_string(const void* c)
        {
            return utils::to_id(static_cast<const Impl*>(c)->context->socket.local_endpoint());
        }

    public:
        explicit Impl(Options init_options)
            : options(std::move(init_options))
        {
            buffer.resize(options.buffer);
        }

        ~Impl() noexcept override = default;

        Impl(Impl&&) noexcept = delete;
        Impl& operator=(Impl&&) noexcept = delete;

        Impl(const Impl&) = delete;
        Impl& operator=(const Impl&) = delete;

        void start() override
        {
            if (!context)
            {
                context = std::make_unique<Context>(options.host, options.port);
                utils::invoke(on_ready_cb, ID{this, this, &Impl::to_string});
                receive();
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
                [this, i = utils::to_array(utils::UDP_EXTERNAL_MESSAGE), msg = std::move(data)]()
                {
                    const auto b = std::array<boost::asio::const_buffer, 2>{
                        boost::asio::buffer(i),
                        boost::asio::buffer(msg)
                    };
                    for (const auto& connection : connections)
                    {
                        context->socket.send_to(b, connection->endpoint);
                    }
                }
            );
        }

        void publish_ex(std::vector<ID> ids, std::vector<std::uint8_t> data) override
        {
            boost::asio::post(
                context->strand,
                [this,
                 ids = std::move(ids),
                 i = utils::to_array(utils::UDP_EXTERNAL_MESSAGE),
                 msg = std::move(data)]()
                {
                    const auto b = std::array<boost::asio::const_buffer, 2>{
                        boost::asio::buffer(i),
                        boost::asio::buffer(msg)
                    };
                    for (const auto& connection : connections)
                    {
                        if (ids.end()
                            == std::find_if(
                                ids.begin(),
                                ids.end(),
                                [&](const auto& id)
                                { return id.owner == this && id.id == connection.get(); }
                            ))
                        {
                            continue;
                        }

                        context->socket.send_to(b, connection->endpoint);
                    }
                }
            );
        }

        void send(std::vector<ID> ids, std::vector<std::uint8_t> data) override
        {
            boost::asio::post(
                context->strand,
                [this,
                 ids = std::move(ids),
                 i = utils::to_array(utils::UDP_EXTERNAL_MESSAGE),
                 msg = std::move(data)]()
                {
                    const auto b = std::array<boost::asio::const_buffer, 2>{
                        boost::asio::buffer(i),
                        boost::asio::buffer(msg)
                    };
                    for (const auto& connection : connections)
                    {
                        auto it = std::find_if(
                            ids.begin(),
                            ids.end(),
                            [&](const auto& id)
                            { return id.owner == this && connection.get() == id.id; }
                        );

                        if (it != ids.end())
                        {
                            context->socket.send_to(b, connection->endpoint);
                        }
                    }
                }
            );
        }

    private:
        struct Connection final
        {
            boost::asio::ip::udp::endpoint endpoint;
            boost::asio::steady_timer timer;

            Connection(
                boost::asio::ip::udp::endpoint init_endpoint,
                boost::asio::strand<boost::asio::io_context::executor_type>& strand
            )
                : endpoint(std::move(init_endpoint))
                , timer(strand)
            {
            }

            static std::string to_string(const void* c)
            {
                return utils::to_id(static_cast<const Connection*>(c)->endpoint);
            }
        };

        Options options;
        std::unique_ptr<Context> context;
        std::vector<std::unique_ptr<Connection>> connections;
        std::vector<std::uint8_t> buffer;

        std::vector<std::function<void(ID, const void*, std::uint64_t)>> on_message_cb;
        std::vector<std::function<void(ID)>> on_ready_cb;
        std::vector<std::function<void(ID)>> on_connect_cb;
        std::vector<std::function<void(ID)>> on_disconnect_cb;

        void ping(const boost::asio::ip::udp::endpoint& endpoint, Connection* connection)
        {
            if (connection == nullptr)
            {
                connections.emplace_back(std::make_unique<Connection>(endpoint, context->strand));
                connection = connections.back().get();
                utils::invoke(on_connect_cb, ID{this, connection, &Connection::to_string});
            }

            connection->timer.expires_after(options.timeout);
            connection->timer.async_wait(
                [this, connection](const boost::system::error_code& ec)
                {
                    if (ec == boost::asio::error::operation_aborted)
                    {
                        return;
                    }
                    utils::invoke(on_disconnect_cb, ID{this, connection, &Connection::to_string});
                    connections.erase(
                        std::remove_if(
                            connections.begin(),
                            connections.end(),
                            [&](const std::unique_ptr<Connection>& cc)
                            { return connection == cc.get(); }
                        ),
                        connections.end()
                    );
                }
            );

            context->socket.send_to(
                boost::asio::buffer(utils::to_array(utils::UDP_INTERNAL_MESSAGE)),
                endpoint
            );
        }

        void message(
            const boost::asio::ip::udp::endpoint& endpoint,
            const std::uint8_t* data,
            std::uint64_t size
        )
        {
            if (size >= sizeof(std::uint8_t))
            {
                auto it = std::find_if(
                    connections.begin(),
                    connections.end(),
                    [&](const auto& connection) { return connection->endpoint == endpoint; }
                );
                auto* connection = (it == connections.end()) ? nullptr : it->get();

                if (utils::from_array<std::uint8_t>(data) == utils::UDP_INTERNAL_MESSAGE)
                {
                    ping(endpoint, connection);
                    return;
                }

                if (connection != nullptr)
                {
                    utils::invoke(
                        on_message_cb,
                        ID{this, connection, &Connection::to_string},
                        data + sizeof(std::uint8_t),
                        size - sizeof(std::uint8_t)
                    );
                }
            }
        }

        void receive()
        {
            auto receiver = std::make_unique<boost::asio::ip::udp::endpoint>();
            auto& capture = *receiver;
            context->socket.async_receive_from(
                boost::asio::buffer(buffer),
                capture,
                [this, receiver = std::move(receiver)](
                    const boost::system::error_code& ec,
                    std::size_t count //
                )
                {
                    if (!ec)
                    {
                        message(*receiver, buffer.data(), count);
                        receive();
                    }
                }
            );
        }

        void impl_on_message(std::function<void(ID, const void*, std::uint64_t)> handler
        ) override
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
