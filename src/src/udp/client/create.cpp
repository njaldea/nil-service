#include <nil/service/udp/client/create.hpp>

#include "../../structs/StandaloneService.hpp"
#include "../../utils.hpp"

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
        }

        boost::asio::io_context ctx;
        boost::asio::strand<boost::asio::io_context::executor_type> strand = make_strand(ctx);
        boost::asio::ip::udp::socket socket;
        boost::asio::steady_timer pingtimer;
        boost::asio::steady_timer timeout;
    };

    struct Impl final: StandaloneService
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

                detail::invoke(handlers.on_ready, utils::to_id(context->socket.local_endpoint()));
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

        void publish_ex(const ID& id, std::vector<std::uint8_t> data) override
        {
            if (context)
            {
                boost::asio::post(
                    context->strand,
                    [this, id, msg = std::move(data)]()
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

        void send(const ID& id, std::vector<std::uint8_t> data) override
        {
            if (targetID == id)
            {
                publish(std::move(data));
            }
        }

        void send(const std::vector<ID>& ids, std::vector<std::uint8_t> data) override
        {
            auto it = std::find(ids.begin(), ids.end(), targetID);
            if (it != ids.end())
            {
                publish(std::move(data));
            }
        }

    private:
        void usermsg(const std::uint8_t* data, std::uint64_t size)
        {
            detail::invoke(handlers.on_message, targetID, data, size);
        }

        void pong()
        {
            if (!connected)
            {
                connected = true;
                detail::invoke(handlers.on_connect, targetID);
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
                        detail::invoke(handlers.on_disconnect, targetID);
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

        Options options;
        std::unique_ptr<Context> context;

        std::vector<std::uint8_t> buffer;

        bool connected = false;
        ID targetID;
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
