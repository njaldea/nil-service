#include <nil/service/udp/client/create.hpp>

#include "../../utils.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>

#include <algorithm>

namespace nil::service::udp::client
{
    struct Context
    {
        explicit Context()
            : strand(make_strand(ctx))
            , socket(strand)
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

    struct Impl final: IStandaloneService
    {
        static std::string to_string_local(const void* c)
        {
            return utils::to_id(static_cast<const Impl*>(c)->context->socket.local_endpoint());
        }

        static std::string to_string_remote(const void* c)
        {
            const auto* impl = static_cast<const Impl*>(c);
            return impl->options.host + ":" + std::to_string(impl->options.port);
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
                context = std::make_unique<Context>();
                if (!open_socket())
                {
                    return;
                }
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

        void dispatch(std::function<void()> task) override
        {
            if (context)
            {
                boost::asio::dispatch(context->ctx, std::move(task));
            }
        }

        void publish(std::vector<std::uint8_t> data) override
        {
            if (context)
            {
                boost::asio::post(
                    context->strand,
                    [this, msg = std::move(data)]() { send_external(msg); }
                );
            }
        }

        void publish_ex(std::vector<ID> ids, std::vector<std::uint8_t> data) override
        {
            if (context)
            {
                boost::asio::post(
                    context->strand,
                    [this, ids = std::move(ids), msg = std::move(data)]()
                    {
                        if (!contains_remote_id(ids))
                        {
                            return;
                        }
                        send_external(msg);
                    }
                );
            }
        }

        void send(std::vector<ID> ids, std::vector<std::uint8_t> data) override
        {
            auto it = std::find(ids.begin(), ids.end(), ID{this, this, &Impl::to_string_remote});
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

        std::vector<std::function<void(ID, const void*, std::uint64_t)>> on_message_cb;
        std::vector<std::function<void(ID)>> on_ready_cb;
        std::vector<std::function<void(ID)>> on_connect_cb;
        std::vector<std::function<void(ID)>> on_disconnect_cb;

        [[nodiscard]] bool open_socket()
        {
            boost::system::error_code ec;
            const auto address = boost::asio::ip::make_address(options.host, ec);
            if (ec)
            {
                return false;
            }

            context->socket.open(
                address.is_v6() ? boost::asio::ip::udp::v6() : boost::asio::ip::udp::v4(),
                ec
            );
            if (ec)
            {
                return false;
            }

            context->socket.set_option(boost::asio::socket_base::reuse_address(true), ec);
            return !ec;
        }

        [[nodiscard]] boost::asio::ip::udp::endpoint remote_endpoint() const
        {
            return {boost::asio::ip::make_address(options.host), options.port};
        }

        [[nodiscard]] bool contains_remote_id(const std::vector<ID>& ids) const
        {
            return ids.end()
                != std::find(ids.begin(), ids.end(), ID{this, this, &Impl::to_string_remote});
        }

        void send_external(const std::vector<std::uint8_t>& msg)
        {
            const auto marker = utils::to_array(utils::UDP_EXTERNAL_MESSAGE);
            context->socket.send_to(
                std::array<boost::asio::const_buffer, 2>{
                    boost::asio::buffer(marker),
                    boost::asio::buffer(msg)
                },
                remote_endpoint()
            );
        }

        void usermsg(const std::uint8_t* data, std::uint64_t size)
        {
            utils::invoke(on_message_cb, ID{this, this, &Impl::to_string_remote}, data, size);
        }

        void pong()
        {
            if (!connected)
            {
                connected = true;
                utils::invoke(on_ready_cb, ID{this, this, &Impl::to_string_local});
                utils::invoke(on_connect_cb, ID{this, this, &Impl::to_string_remote});
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
                        utils::invoke(on_disconnect_cb, ID{this, this, &Impl::to_string_remote});
                        recreate_socket();
                    }
                }
            );
        }

        void recreate_socket()
        {
            boost::system::error_code ec;
            context->socket.cancel(ec);
            context->socket.close(ec);

            if (!open_socket())
            {
                return;
            }

            receive();
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
