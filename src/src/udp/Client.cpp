#include <nil/service/udp/Client.hpp>

#include "../utils.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>

namespace nil::service::udp
{
    struct Client::Impl final
    {
        explicit Impl(const Options& init_options, const detail::Handlers& init_handlers)
            : options(init_options)
            , handlers(init_handlers)
            , strand(boost::asio::make_strand(context))
            , socket(strand, {boost::asio::ip::make_address("0.0.0.0"), 0})
            , pingtimer(strand)
            , timeout(strand)
            , targetID{options.host + ":" + std::to_string(options.port)}
        {
            buffer.resize(options.buffer);
        }

        ~Impl() noexcept = default;

        Impl(Impl&&) noexcept = delete;
        Impl& operator=(Impl&&) noexcept = delete;

        Impl(const Impl&) = delete;
        Impl& operator=(const Impl&) = delete;

        void publish(std::vector<std::uint8_t> data)
        {
            boost::asio::dispatch(
                strand,
                [this, msg = std::move(data)]()
                {
                    socket.send_to(
                        std::array<boost::asio::const_buffer, 2>{
                            boost::asio::buffer(utils::to_array(utils::UDP_EXTERNAL_MESSAGE)),
                            boost::asio::buffer(msg)
                        },
                        {boost::asio::ip::make_address(options.host), options.port}
                    );
                }
            );
        }

        void usermsg(const std::uint8_t* data, std::uint64_t size)
        {
            if (handlers.msg)
            {
                handlers.msg->call(targetID, data, size);
            }
        }

        void pong()
        {
            if (!connected)
            {
                connected = true;
                if (handlers.connect)
                {
                    handlers.connect->call(targetID);
                }
            }
            timeout.expires_after(options.timeout);
            timeout.async_wait(
                [this](const boost::system::error_code& ec)
                {
                    if (ec == boost::asio::error::operation_aborted)
                    {
                        return;
                    }
                    if (connected)
                    {
                        connected = false;
                        if (handlers.disconnect)
                        {
                            handlers.disconnect->call(targetID);
                        }
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
            socket.async_receive(
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
            socket.send_to(
                boost::asio::buffer(utils::to_array(utils::UDP_INTERNAL_MESSAGE)),
                {boost::asio::ip::make_address(options.host), options.port}
            );
            pingtimer.expires_after(options.timeout / 2);
            pingtimer.async_wait(
                [this](const boost::system::error_code& ec)
                {
                    if (ec != boost::asio::error::operation_aborted)
                    {
                        ping();
                    }
                }
            );
        }

        void run()
        {
            if (handlers.ready)
            {
                handlers.ready->call(
                    {socket.local_endpoint().address().to_string() + ":"
                     + std::to_string(socket.local_endpoint().port())}
                );
            }
            ping();
            receive();
            context.run();
        }

        const Options& options;
        const detail::Handlers& handlers;

        boost::asio::io_context context;
        boost::asio::strand<boost::asio::io_context::executor_type> strand;
        boost::asio::ip::udp::socket socket;
        boost::asio::steady_timer pingtimer;
        boost::asio::steady_timer timeout;

        std::vector<std::uint8_t> buffer;

        bool connected = false;
        ID targetID;
    };

    Client::Client(Client::Options init_options)
        : options{std::move(init_options)}
        , impl(std::make_unique<Impl>(options, handlers))
    {
    }

    Client::~Client() noexcept = default;

    void Client::run()
    {
        impl->run();
    }

    void Client::stop()
    {
        impl->context.stop();
    }

    void Client::restart()
    {
        impl.reset();
        impl = std::make_unique<Impl>(options, handlers);
    }

    void Client::send(const ID& id, std::vector<std::uint8_t> data)
    {
        if (impl->targetID == id)
        {
            impl->publish(std::move(data));
        }
    }

    void Client::publish(std::vector<std::uint8_t> data)
    {
        impl->publish(std::move(data));
    }
}
