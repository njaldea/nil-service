#include <nil/service/self/create.hpp>

#include "../utils.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>

namespace nil::service::self
{
    struct Impl final: IStandaloneService
    {
    public:
        void publish(std::vector<std::uint8_t> payload) override
        {
            if (context)
            {
                boost::asio::post(
                    *context,
                    [this, msg = std::move(payload)]()
                    { utils::invoke(on_message_cb, self, msg.data(), msg.size()); }
                );
            }
        }

        void publish_ex(ID id, std::vector<std::uint8_t> payload) override
        {
            if (context)
            {
                boost::asio::post(
                    *context,
                    [this, id = std::move(id), msg = std::move(payload)]()
                    {
                        if (this->self != id)
                        {
                            utils::invoke(on_message_cb, self, msg.data(), msg.size());
                        }
                    }
                );
            }
        }

        void send(ID to, std::vector<std::uint8_t> payload) override
        {
            if (self == to)
            {
                publish(std::move(payload));
            }
        }

        void send(std::vector<ID> ids, std::vector<std::uint8_t> data) override
        {
            auto it = std::find(ids.begin(), ids.end(), self);
            if (it != ids.end())
            {
                this->send(*it, std::move(data));
            }
        }

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

        void start() override
        {
            if (!context)
            {
                context = std::make_unique<boost::asio::io_context>();
                boost::asio::post(
                    *context,
                    [this]()
                    {
                        utils::invoke(on_ready_cb, self);
                        utils::invoke(on_connect_cb, self);
                    }
                );
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

    private:
        ID self = {"self"};

        std::unique_ptr<boost::asio::io_context> context;
        std::vector<std::function<void(const ID&, const void*, std::uint64_t)>> on_message_cb;
        std::vector<std::function<void(const ID&)>> on_ready_cb;
        std::vector<std::function<void(const ID&)>> on_connect_cb;
        std::vector<std::function<void(const ID&)>> on_disconnect_cb;
    };

    std::unique_ptr<IStandaloneService> create()
    {
        return std::make_unique<Impl>();
    }
}
