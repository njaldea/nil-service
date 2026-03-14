#include <nil/service/gateway/create.hpp>

#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>

namespace nil::service::gateway
{
    class Impl final: public IGatewayService
    {
    public:
        Impl() = default;

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

        void impl_on_message(std::function<void(const ID&, const void*, std::uint64_t)> handler
        ) override
        {
            for (auto* service : services)
            {
                service->on_message(
                    [this, handler](const ID& id, const void* data, std::uint64_t size)
                    {
                        const auto* start = static_cast<const std::uint8_t*>(data);
                        const auto* end = start + size;
                        this->dispatch([id, d = std::vector<std::uint8_t>(start, end), handler]()
                                       { handler(id, d.data(), d.size()); });
                    }
                );
            }
        }

        void impl_on_ready(std::function<void(const ID&)> handler) override
        {
            for (auto* service : services)
            {
                service->on_ready([this, handler](const ID& id)
                                  { this->dispatch([id, handler]() { handler(id); }); });
            }
        }

        void impl_on_connect(std::function<void(const ID&)> handler) override
        {
            for (auto* service : services)
            {
                service->on_connect([this, handler](const ID& id)
                                    { this->dispatch([id, handler]() { handler(id); }); });
            }
        }

        void impl_on_disconnect(std::function<void(const ID&)> handler) override
        {
            for (auto* service : services)
            {
                service->on_disconnect([this, handler](const ID& id)
                                       { this->dispatch([id, handler]() { handler(id); }); });
            }
        }

        void add_service(IService& service) override
        {
            services.push_back(&service);
        }

        void start() override
        {
            if (!context)
            {
                context = std::make_unique<boost::asio::io_context>();
            }
            auto _ = boost::asio::make_work_guard(*context);
            context->run();
        }

        void stop() override
        {
            if (context)
            {
                context->stop();
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
                boost::asio::post(*context, std::move(task));
            }
        }

    private:
        std::vector<IService*> services;
        std::unique_ptr<boost::asio::io_context> context;
    };

    std::unique_ptr<IGatewayService> create()
    {
        return std::make_unique<Impl>();
    }
}
