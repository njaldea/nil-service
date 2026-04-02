#include <nil/service/self/create.hpp>

#include "../utils.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>

#include <algorithm>

namespace nil::service::self
{
    struct Impl final: IStandaloneService
    {
        static std::string to_string(const void* c)
        {
            (void)c;
            return "self";
        }

    public:
        void publish(std::vector<std::uint8_t> payload) override
        {
            if (context)
            {
                boost::asio::post(
                    *context,
                    [this, msg = std::move(payload)]() {
                        utils::invoke(
                            on_message_cb,
                            ID{this, this, &to_string},
                            msg.data(),
                            msg.size()
                        );
                    }
                );
            }
        }

        void publish_ex(std::vector<ID> ids, std::vector<std::uint8_t> payload) override
        {
            if (context)
            {
                boost::asio::post(
                    *context,
                    [this, ids = std::move(ids), msg = std::move(payload)]()
                    {
                        if (ids.end()
                            == std::find(ids.begin(), ids.end(), ID{this, this, &to_string}))
                        {
                            return;
                        }
                        utils::invoke(
                            on_message_cb,
                            ID{this, this, &to_string},
                            msg.data(),
                            msg.size()
                        );
                    }
                );
            }
        }

        void send(std::vector<ID> ids, std::vector<std::uint8_t> data) override
        {
            if (ids.end() != std::find(ids.begin(), ids.end(), ID{this, this, &to_string}))
            {
                utils::invoke(on_message_cb, ID{this, this, &to_string}, data.data(), data.size());
            }
        }

        void impl_on_message(std::function<void(ID, const void*, std::uint64_t)> handler
        ) override
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

        void start() override
        {
            if (!context)
            {
                context = std::make_unique<boost::asio::io_context>();
                boost::asio::post(
                    *context,
                    [this]()
                    {
                        const auto id = ID{this, this, &to_string};
                        utils::invoke(on_ready_cb, id);
                        utils::invoke(on_connect_cb, id);
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

        void dispatch(std::function<void()> task) override
        {
            if (context)
            {
                boost::asio::post(*context, std::move(task));
            }
        }

    private:
        std::unique_ptr<boost::asio::io_context> context;
        std::vector<std::function<void(ID, const void*, std::uint64_t)>> on_message_cb;
        std::vector<std::function<void(ID)>> on_ready_cb;
        std::vector<std::function<void(ID)>> on_connect_cb;
        std::vector<std::function<void(ID)>> on_disconnect_cb;
    };

    std::unique_ptr<IStandaloneService> create()
    {
        return std::make_unique<Impl>();
    }
}
