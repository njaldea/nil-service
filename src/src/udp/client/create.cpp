#include <nil/service/udp/client/create.hpp>

#include "../../utils.hpp"

#define BOOST_ASIO_STANDALONE
#define BOOST_ASIO_NO_TYPEID
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>

namespace nil::service::udp::client
{
    struct Context
    {
        explicit Context()
            : strand(make_strand(ctx))
            , socket(strand, {boost::asio::ip::make_address("0.0.0.0"), 0})
            , pingtimer(strand)
            , timeout(strand)
        {
            socket.set_option(boost::asio::socket_base::reuse_address(true));
        }

        boost::asio::io_context ctx;
        boost::asio::strand<boost::asio::io_context::executor_type> strand = make_strand(ctx);
        boost::asio::ip::udp::socket socket;
        boost::asio::steady_timer pingtimer;
        boost::asio::steady_timer timeout;
    };

    struct Impl final: IStandaloneService
    {
    public:
        explicit Impl(Options init_options)
            : options(std::move(init_options))
            , targetID{options.host + ":" + std::to_string(options.port)}
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
                context = std::make_unique<Context>();

                utils::invoke(on_ready_cb, utils::to_id(context->socket.local_endpoint()));
                ping();
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
            if (context)
            {
                boost::asio::post(
                    context->strand,
                    [this, msg = std::move(data)]()
                    {
                        const auto header
                            = boost::asio::buffer(utils::to_array(utils::UDP_EXTERNAL_MESSAGE));
                        context->socket.send_to(
                            std::array<boost::asio::const_buffer, 2>{
                                header,
                                boost::asio::buffer(msg)
                            },
                            {boost::asio::ip::make_address(options.host), options.port}
                        );
                    }
                );
            }
        }

        void publish_ex(ID id, std::vector<std::uint8_t> data) override
        {
            if (context)
            {
                boost::asio::post(
                    context->strand,
                    [this, id = std::move(id), msg = std::move(data)]()
                    {
                        if (targetID != id)
                        {
                            const auto header
                                = boost::asio::buffer(utils::to_array(utils::UDP_EXTERNAL_MESSAGE));
                            context->socket.send_to(
                                std::array<boost::asio::const_buffer, 2>{
                                    header,
                                    boost::asio::buffer(msg)
                                },
                                {boost::asio::ip::make_address(options.host), options.port}
                            );
                        }
                    }
                );
            }
        }

        void send(ID id, std::vector<std::uint8_t> data) override
        {
            if (targetID == id)
            {
                publish(std::move(data));
            }
        }

        void send(std::vector<ID> ids, std::vector<std::uint8_t> data) override
        {
            auto it = std::find(ids.begin(), ids.end(), targetID);
            if (it != ids.end())
            {
                publish(std::move(data));
            }
        }

    private:
        Options options;
        std::unique_ptr<Context> context;

        std::vector<std::uint8_t> buffer;

        bool connected = false;
        ID targetID;

        std::vector<std::function<void(const ID&, const void*, std::uint64_t)>> on_message_cb;
        std::vector<std::function<void(const ID&)>> on_ready_cb;
        std::vector<std::function<void(const ID&)>> on_connect_cb;
        std::vector<std::function<void(const ID&)>> on_disconnect_cb;

        void usermsg(const std::uint8_t* data, std::uint64_t size)
        {
            utils::invoke(on_message_cb, targetID, data, size);
        }

        void pong()
        {
            if (!connected)
            {
                connected = true;
                utils::invoke(on_connect_cb, targetID);
            }
            context->timeout.expires_after(options.timeout);
            context->timeout.async_wait(
                [this](const boost::system::error_code& ec)
                {
                    if (ec == boost::asio::error::operation_aborted)
                    {
                        return;
                    }
                    if (connected)
                    {
                        connected = false;
                        utils::invoke(on_disconnect_cb, targetID);
                    }
                }
            );
        }

        void message(const std::uint8_t* data, std::uint64_t size)
        {
            if (size >= sizeof(std::uint8_t))
            {
                if (utils::from_array<std::uint8_t>(data) > 0u)
                {
                    pong();
                }
                else
                {
                    usermsg(data + sizeof(std::uint8_t), size - sizeof(std::uint8_t));
                }
            }
        }

        void receive()
        {
            context->socket.async_receive(
                boost::asio::buffer(buffer),
                [this](const boost::system::error_code& ec, std::size_t count)
                {
                    if (ec)
                    {
                        return;
                    }

                    message(buffer.data(), count);
                    receive();
                }
            );
        }

        void ping()
        {
            context->socket.send_to(
                boost::asio::buffer(utils::to_array(utils::UDP_INTERNAL_MESSAGE)),
                {boost::asio::ip::make_address(options.host), options.port}
            );
            context->pingtimer.expires_after(options.timeout / 2);
            context->pingtimer.async_wait(
                [this](const boost::system::error_code& ec)
                {
                    if (ec != boost::asio::error::operation_aborted)
                    {
                        ping();
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
