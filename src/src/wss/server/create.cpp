#include <nil/service/wss/server/create.hpp>

#include <nil/service/https/server/create.hpp>

#include "../../structs/StandaloneService.hpp"
#include "../../structs/WebService.hpp"

namespace nil::service::wss::server
{
    struct Impl final: StandaloneService
    {
    public:
        explicit Impl(Options options)
            : server(https::server::create(
                  {.cert = std::move(options.cert),
                   .host = std::move(options.host),
                   .port = options.port,
                   .buffer = options.buffer}
              ))
            , ws(use_ws(server, options.route))
        {
        }

        void publish(std::vector<std::uint8_t> payload) override
        {
            ws.ptr->publish(std::move(payload));
        }

        void publish_ex(ID id, std::vector<std::uint8_t> payload) override
        {
            ws.ptr->publish_ex(std::move(id), std::move(payload));
        }

        void send(ID id, std::vector<std::uint8_t> payload) override
        {
            ws.ptr->send(std::move(id), std::move(payload));
        }

        void send(std::vector<ID> ids, std::vector<std::uint8_t> payload) override
        {
            ws.ptr->send(std::move(ids), std::move(payload));
        }

        void start() override
        {
            ws.ptr->handlers = handlers;
            server.ptr->start();
        }

        void stop() override
        {
            server.ptr->stop();
        }

        void restart() override
        {
            server.ptr->restart();
        }

    private:
        W server;
        P ws;
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
