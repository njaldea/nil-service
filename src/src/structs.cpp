#include <nil/service/structs.hpp>

#include "structs/HTTPService.hpp"
#include "structs/ObservableService.hpp"
#include "structs/WebService.hpp"

namespace nil::service
{
    namespace impl
    {
        void on_ready(ObservableService& service, std::function<void(const ID&)> handler)
        {
            service.handlers.on_ready.push_back(std::move(handler));
        }

        void on_connect(ObservableService& service, std::function<void(const ID&)> handler)
        {
            service.handlers.on_connect.push_back(std::move(handler));
        }

        void on_disconnect(ObservableService& service, std::function<void(const ID&)> handler)
        {
            service.handlers.on_disconnect.push_back(std::move(handler));
        }

        void on_message(
            ObservableService& service,
            std::function<void(const ID&, const void*, std::uint64_t)> handler
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

    void publish_ex(MessagingService& service, ID id, const void* data, std::uint64_t size)
    {
        const auto* ptr = static_cast<const std::uint8_t*>(data);
        service.publish_ex(std::move(id), std::vector<std::uint8_t>(ptr, ptr + size));
    }

    void send(MessagingService& service, ID id, const void* data, std::uint64_t size)
    {
        const auto* ptr = static_cast<const std::uint8_t*>(data);
        service.send(std::move(id), std::vector<std::uint8_t>(ptr, ptr + size));
    }

    void send(
        MessagingService& service,
        const std::vector<ID>& ids,
        const void* data,
        std::uint64_t size
    )
    {
        const auto* ptr = static_cast<const std::uint8_t*>(data);
        service.send(ids, std::vector<std::uint8_t>(ptr, ptr + size));
    }

    void publish(MessagingService& service, std::vector<std::uint8_t> payload)
    {
        service.publish(std::move(payload));
    }

    void publish_ex(MessagingService& service, ID id, std::vector<std::uint8_t> payload)
    {
        service.publish_ex(std::move(id), std::move(payload));
    }

    void send(MessagingService& service, ID id, std::vector<std::uint8_t> payload)
    {
        service.send(std::move(id), std::move(payload));
    }

    void send(MessagingService& service, std::vector<ID> ids, std::vector<std::uint8_t> payload)
    {
        service.send(std::move(ids), std::move(payload));
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
        void on_ready(WebService& service, std::function<void(const ID&)> handler)
        {
            service.on_ready.push_back(std::move(handler));
        }
    }

    void on_get(WebService& service, std::function<void(const WebTransaction&)> callback)
    {
        service.on_get = std::move(callback);
    }

    P use_ws(WebService& service, const std::string& route)
    {
        return {service.get_wss(route)};
    }
}
