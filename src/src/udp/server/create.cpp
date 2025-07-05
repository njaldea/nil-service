#include <nil/service/udp/server/create.hpp>

#include "../../structs/StandaloneService.hpp"
#include "../../utils.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>

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

    struct Impl final: StandaloneService
    {
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
                detail::invoke(handlers.on_ready, utils::to_id(context->socket.local_endpoint()));
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

        void publish(std::vector<std::uint8_t> data) override
        {
            boost::asio::post(
                context->strand,
                [this, i = utils::to_array(utils::UDP_EXTERNAL_MESSAGE), msg = std::move(data)]()
                {
                    const auto b = std::array<boost::asio::const_buffer, 3>{
                        boost::asio::buffer(i),
                        boost::asio::buffer(msg)
                    };
                    for (const auto& connection : connections)
                    {
                        context->socket.send_to(b, connection.second->endpoint);
                    }
                }
            );
        }

        void publish_ex(ID id, std::vector<std::uint8_t> data) override
        {
            boost::asio::post(
                context->strand,
                [this,
                 id = std::move(id),
                 i = utils::to_array(utils::UDP_EXTERNAL_MESSAGE),
                 msg = std::move(data)]()
                {
                    const auto b = std::array<boost::asio::const_buffer, 3>{
                        boost::asio::buffer(i),
                        boost::asio::buffer(msg)
                    };
                    for (const auto& connection : connections)
                    {
                        if (connection.first != id)
                        {
                            context->socket.send_to(b, connection.second->endpoint);
                        }
                    }
                }
            );
        }

        void send(ID id, std::vector<std::uint8_t> data) override
        {
            boost::asio::post(
                context->strand,
                [this,
                 id = std::move(id),
                 i = utils::to_array(utils::UDP_EXTERNAL_MESSAGE),
                 msg = std::move(data)]()
                {
                    const auto b = std::array<boost::asio::const_buffer, 2>{
                        boost::asio::buffer(i),
                        boost::asio::buffer(msg)
                    };
                    auto it = connections.find(id);
                    if (it != connections.end())
                    {
                        context->socket.send_to(b, it->second->endpoint);
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
                    for (const auto& id : ids)
                    {
                        auto it = connections.find(id);
                        if (it != connections.end())
                        {
                            context->socket.send_to(b, it->second->endpoint);
                        }
                    }
                }
            );
        }

    private:
        void ping(const boost::asio::ip::udp::endpoint& endpoint, const ID& id)
        {
            auto& connection = connections[id];
            if (!connection)
            {
                connection = std::make_unique<Connection>(endpoint, context->strand);
                detail::invoke(handlers.on_connect, id);
            }
            connection->timer.expires_after(options.timeout);
            connection->timer.async_wait(
                [this, id](const boost::system::error_code& ec)
                {
                    if (ec == boost::asio::error::operation_aborted)
                    {
                        return;
                    }
                    detail::invoke(handlers.on_disconnect, id);
                    connections.erase(id);
                }
            );

            context->socket.send_to(
                boost::asio::buffer(utils::to_array(utils::UDP_INTERNAL_MESSAGE)),
                endpoint
            );
        }

        void usermsg(const ID& id, const std::uint8_t* data, std::uint64_t size)
        {
            detail::invoke(handlers.on_message, id, data, size);
        }

        void message(
            const boost::asio::ip::udp::endpoint& endpoint,
            const std::uint8_t* data,
            std::uint64_t size
        )
        {
            if (size >= sizeof(std::uint8_t))
            {
                if (utils::from_array<std::uint8_t>(data) > 0u)
                {
                    ping(endpoint, {utils::to_id(endpoint)});
                }
                else
                {
                    usermsg(
                        {utils::to_id(endpoint)},
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
        };

        using Connections = std::unordered_map<ID, std::unique_ptr<Connection>>;

        Options options;
        std::unique_ptr<Context> context;
        Connections connections;
        std::vector<std::uint8_t> buffer;
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
