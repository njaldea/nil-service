#include <nil/service/self/create.hpp>

#include "../structs/StandaloneService.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>

namespace nil::service::self
{
    struct Impl final: StandaloneService
    {
        std::unique_ptr<boost::asio::io_context> context;

        ID id = {"self"};

        void publish(std::vector<std::uint8_t> payload) override
        {
            if (context)
            {
                boost::asio::post(
                    *context,
                    [this, msg = std::move(payload)]()
                    { detail::invoke(handlers.on_message, id, msg.data(), msg.size()); }
                );
            }
        }

        void send(const ID& to, std::vector<std::uint8_t> payload) override
        {
            if (id == to)
            {
                publish(std::move(payload));
            }
        }

        void send(const std::vector<ID>& ids, std::vector<std::uint8_t> data) override
        {
            auto it = std::find(ids.begin(), ids.end(), id);
            if (it != ids.end())
            {
                this->send(*it, std::move(data));
            }
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
                        detail::invoke(handlers.on_ready, id);
                        detail::invoke(handlers.on_connect, id);
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

        void exec(std::unique_ptr<detail::ICallable<>> executable) override
        {
            boost::asio::post(
                *context,
                [executable = std::move(executable)]() { executable->call(); }
            );
        }
    };

    A create()
    {
        constexpr auto deleter = [](StandaloneService* obj) { //
            auto ptr = static_cast<Impl*>(obj);               // NOLINT
            std::default_delete<Impl>()(ptr);
        };
        return {{new Impl(), deleter}};
    }
}
