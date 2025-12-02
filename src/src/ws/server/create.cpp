#include <nil/service/ws/server/create.hpp>

#include <nil/service/http/server/create.hpp>

namespace nil::service::ws::server
{
    struct Impl final: IStandaloneService
    {
    public:
        explicit Impl(Options options)
            : server(http::server::create(
                  {.host = std::move(options.host), .port = options.port, .buffer = options.buffer}
              ))
            , ws(server->use_ws(options.route))
        {
        }

        void publish(std::vector<std::uint8_t> payload) override
        {
            ws->publish(std::move(payload));
        }

        void publish_ex(ID id, std::vector<std::uint8_t> payload) override
        {
            ws->publish_ex(std::move(id), std::move(payload));
        }

        void send(ID id, std::vector<std::uint8_t> payload) override
        {
            ws->send(std::move(id), std::move(payload));
        }

        void send(std::vector<ID> ids, std::vector<std::uint8_t> payload) override
        {
            ws->send(std::move(ids), std::move(payload));
        }

        void start() override
        {
            for (const auto& cb : on_message_cb)
            {
                ws->on_message(cb);
            }
            for (const auto& cb : on_connect_cb)
            {
                ws->on_connect(cb);
            }
            for (const auto& cb : on_disconnect_cb)
            {
                ws->on_disconnect(cb);
            }
            for (const auto& cb : on_ready_cb)
            {
                ws->on_ready(cb);
            }
            server->start();
        }

        void stop() override
        {
            server->stop();
        }

        void restart() override
        {
            server->restart();
        }

    private:
        std::unique_ptr<IWebService> server;
        IService* ws;

        std::vector<std::function<void(const ID&, const void*, std::uint64_t)>> on_message_cb;
        std::vector<std::function<void(const ID&)>> on_ready_cb;
        std::vector<std::function<void(const ID&)>> on_connect_cb;
        std::vector<std::function<void(const ID&)>> on_disconnect_cb;

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
