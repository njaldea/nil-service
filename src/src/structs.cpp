#include <nil/service/structs.hpp>

#include "./structs/HTTPService.hpp"
#include "./structs/ObservableService.hpp"

namespace nil::service
{
    namespace impl
    {
        void on_ready(
            ObservableService& service,
            std::unique_ptr<detail::ICallable<const ID&>> handler
        )
        {
            service.handlers.on_ready.push_back(std::move(handler));
        }

        void on_connect(
            ObservableService& service,
            std::unique_ptr<detail::ICallable<const ID&>> handler
        )
        {
            service.handlers.on_connect.push_back(std::move(handler));
        }

        void on_disconnect(
            ObservableService& service,
            std::unique_ptr<detail::ICallable<const ID&>> handler
        )
        {
            service.handlers.on_disconnect.push_back(std::move(handler));
        }

        void on_message(
            ObservableService& service,
            std::unique_ptr<detail::ICallable<const ID&, const void*, std::uint64_t>> handler
        )
        {
            service.handlers.on_message.push_back(std::move(handler));
        }
    }

    void publish(MessagingService& service, const void* data, std::uint64_t size)
    {
        const auto* ptr = static_cast<const std::uint8_t*>(data);
        service.publish(std::vector<std::uint8_t>(ptr, ptr + size));
    }

    void send(MessagingService& service, const ID& id, const void* data, std::uint64_t size)
    {
        const auto* ptr = static_cast<const std::uint8_t*>(data);
        service.send(id, std::vector<std::uint8_t>(ptr, ptr + size));
    }

    void publish(MessagingService& service, std::vector<std::uint8_t> payload)
    {
        service.publish(std::move(payload));
    }

    void send(MessagingService& service, const ID& id, std::vector<std::uint8_t> payload)
    {
        service.send(id, std::move(payload));
    }

    void start(RunnableService& service)
    {
        service.start();
    }

    void stop(RunnableService& service)
    {
        service.stop();
    }

    void restart(RunnableService& service)
    {
        service.restart();
    }

    namespace impl
    {
        void use(
            HTTPService& service,
            std::string route,
            std::string content_type,
            std::unique_ptr<detail::ICallable<std::ostream&>> body
        )
        {
            service.routes.emplace(
                std::move(route),
                http::server::Route{std::move(content_type), std::move(body)}
            );
        }

        void on_ready(HTTPService& service, std::unique_ptr<detail::ICallable<const ID&>> handler)
        {
            service.on_ready.push_back(std::move(handler));
        }
    }

    S use_ws(HTTPService& service, std::string route)
    {
        return {&service.wss[std::move(route)]};
    }
}
