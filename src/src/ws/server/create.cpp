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
            attach_callbacks();
        }

        void publish(std::vector<std::uint8_t> payload) override
        {
            ws->publish(std::move(payload));
        }

        void publish_ex(std::vector<ID> ids, std::vector<std::uint8_t> payload) override
        {
            ws->publish_ex(std::move(ids), std::move(payload));
        }

        void send(std::vector<ID> ids, std::vector<std::uint8_t> payload) override
        {
            ws->send(std::move(ids), std::move(payload));
        }

        void run() override
        {
            server->run();
        }

        void poll() override
        {
            server->poll();
        }

        void stop() override
        {
            server->stop();
        }

        void restart() override
        {
            server->restart();
        }

        void dispatch(std::function<void()> task) override
        {
            server->dispatch(std::move(task));
        }

    private:
        std::unique_ptr<IWebService> server;
        IEventService* ws;

        std::vector<std::function<void(ID, const void*, std::uint64_t)>> on_message_cb;
        std::vector<std::function<void(ID)>> on_ready_cb;
        std::vector<std::function<void(ID)>> on_connect_cb;
        std::vector<std::function<void(ID)>> on_disconnect_cb;

        void attach_callbacks()
        {
            ws->on_message(
                [this](ID id, const void* data, std::uint64_t size)
                {
                    for (const auto& cb : on_message_cb)
                    {
                        cb(id, data, size);
                    }
                }
            );
            ws->on_connect(
                [this](ID id)
                {
                    for (const auto& cb : on_connect_cb)
                    {
                        cb(id);
                    }
                }
            );
            ws->on_disconnect(
                [this](ID id)
                {
                    for (const auto& cb : on_disconnect_cb)
                    {
                        cb(id);
                    }
                }
            );
            ws->on_ready(
                [this](ID id)
                {
                    for (const auto& cb : on_ready_cb)
                    {
                        cb(id);
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
