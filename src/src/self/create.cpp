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
                queue_self_message(std::move(payload));
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
                        if (contains_self_id(ids))
                        {
                            return;
                        }
                        emit_self_message(msg);
                    }
                );
            }
        }

        void send(std::vector<ID> ids, std::vector<std::uint8_t> data) override
        {
            if (!context)
            {
                return;
            }

            boost::asio::post(
                *context,
                [this, ids = std::move(ids), msg = std::move(data)]()
                {
                    if (!contains_self_id(ids))
                    {
                        return;
                    }
                    emit_self_message(msg);
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
        [[nodiscard]] ID self_id() const
        {
            return {this, this, &to_string};
        }

        [[nodiscard]] bool contains_self_id(const std::vector<ID>& ids) const
        {
            return ids.end() != std::find(ids.begin(), ids.end(), self_id());
        }

        void emit_self_message(const std::vector<std::uint8_t>& msg)
        {
            const auto id = self_id();
            utils::invoke(on_message_cb, id, msg.data(), msg.size());
        }

        void queue_self_message(std::vector<std::uint8_t> msg)
        {
            boost::asio::post(*context, [this, msg = std::move(msg)]() { emit_self_message(msg); });
        }

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
