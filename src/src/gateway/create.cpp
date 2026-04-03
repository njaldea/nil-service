#include <nil/service/gateway/create.hpp>

#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>

namespace nil::service::gateway
{
    class Impl final: public IGatewayService
    {
    public:
        Impl()
            : context(std::make_unique<boost::asio::io_context>())
        {
        }

        ~Impl() noexcept override = default;

        Impl(Impl&&) = delete;
        Impl(const Impl&) = delete;
        Impl& operator=(Impl&&) = delete;
        Impl& operator=(const Impl&) = delete;

        void publish(std::vector<std::uint8_t> payload) override
        {
            for (auto* service : services)
            {
                service->publish(payload);
            }
        }

        void publish_ex(std::vector<ID> ids, std::vector<std::uint8_t> payload) override
        {
            for (auto* service : services)
            {
                service->publish_ex(ids, payload);
            }
        }

        void send(std::vector<ID> ids, std::vector<std::uint8_t> payload) override
        {
            for (auto* service : services)
            {
                service->send(ids, payload);
            }
        }

        void impl_on_message(std::function<void(ID, const void*, std::uint64_t)> handler) override
        {
            on_message_handlers.push_back(std::move(handler));
        }

        void impl_on_ready(std::function<void(ID)> handler) override
        {
            on_ready_handlers.push_back(std::move(handler));
        }

        void impl_on_connect(std::function<void(ID)> handler) override
        {
            on_connect_handlers.push_back(std::move(handler));
        }

        void impl_on_disconnect(std::function<void(ID)> handler) override
        {
            on_disconnect_handlers.push_back(std::move(handler));
        }

        void add_service(IEventService& service) override
        {
            services.push_back(&service);
            attach_message_forwarder(service);
            attach_event_forwarder(service, &Impl::on_ready_handlers, &IEventService::on_ready);
            attach_event_forwarder(service, &Impl::on_connect_handlers, &IEventService::on_connect);
            attach_event_forwarder(
                service,
                &Impl::on_disconnect_handlers,
                &IEventService::on_disconnect
            );
        }

        void start() override
        {
            context->restart();
            auto _ = boost::asio::make_work_guard(*context);
            context->run();
        }

        void stop() override
        {
            context->stop();
        }

        void restart() override
        {
            context = std::make_unique<boost::asio::io_context>();
        }

        void dispatch(std::function<void()> task) override
        {
            boost::asio::post(*context, std::move(task));
        }

    private:
        using MsgHandler = std::function<void(ID, const void*, std::uint64_t)>;
        using EventHandler = std::function<void(ID)>;

        std::vector<IEventService*> services;
        std::unique_ptr<boost::asio::io_context> context;
        std::vector<MsgHandler> on_message_handlers;
        std::vector<EventHandler> on_ready_handlers;
        std::vector<EventHandler> on_connect_handlers;
        std::vector<EventHandler> on_disconnect_handlers;

        static void invoke_event_handlers(const std::vector<EventHandler>& handlers, const ID& id)
        {
            for (const auto& handler : handlers)
            {
                if (handler)
                {
                    handler(id);
                }
            }
        }

        static void invoke_message_handlers(
            const std::vector<MsgHandler>& handlers,
            const ID& id,
            const std::vector<std::uint8_t>& payload
        )
        {
            for (const auto& handler : handlers)
            {
                if (handler)
                {
                    handler(id, payload.data(), payload.size());
                }
            }
        }

        void attach_message_forwarder(IEventService& service)
        {
            service.on_message(
                [this](ID id, const void* data, std::uint64_t size)
                {
                    const auto* start = static_cast<const std::uint8_t*>(data);
                    const auto* end = start + size;
                    auto payload = std::vector<std::uint8_t>(start, end);
                    this->dispatch([this, id, payload = std::move(payload)]()
                                   { invoke_message_handlers(on_message_handlers, id, payload); });
                }
            );
        }

        void attach_event_forwarder(
            IEventService& service,
            const std::vector<EventHandler> Impl::*handlers,
            void (IEventService::*subscribe)(std::function<void(ID)>)
        )
        {
            (service.*subscribe)(
                [this, handlers](ID id) {
                    this->dispatch([this, handlers, id]()
                                   { invoke_event_handlers(this->*handlers, id); });
                }
            );
        }
    };

    std::unique_ptr<IGatewayService> create()
    {
        return std::make_unique<Impl>();
    }
}
