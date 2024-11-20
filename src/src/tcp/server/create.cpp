#include <nil/service/tcp/server/create.hpp>

#include "../../structs/StandaloneService.hpp"
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
        }

        boost::asio::io_context ctx;
        boost::asio::strand<boost::asio::io_context::executor_type> strand;
        boost::asio::ip::tcp::acceptor acceptor;
    };

    struct Impl final
        : StandaloneService
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
                detail::invoke(handlers.on_ready, utils::to_id(context->acceptor.local_endpoint()));
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

        void send(const ID& id, std::vector<std::uint8_t> data) override
        {
            boost::asio::post(
                context->strand,
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

        void send(const std::vector<ID>& ids, std::vector<std::uint8_t> data) override
        {
            boost::asio::post(
                context->strand,
                [this, ids, msg = std::move(data)]()
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
        void connect(Connection* connection) override
        {
            detail::invoke(handlers.on_connect, connection->id());
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
                    detail::invoke(handlers.on_disconnect, id);
                }
            );
        }

        void message(const ID& id, const void* data, std::uint64_t size) override
        {
            detail::invoke(handlers.on_message, id, data, size);
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

        Options options;
        std::unique_ptr<Context> context;
        std::unordered_map<ID, std::unique_ptr<Connection>> connections;
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
