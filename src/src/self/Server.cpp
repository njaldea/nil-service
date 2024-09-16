#include <nil/service/self/Server.hpp>

#include <boost/asio/executor_work_guard.hpp>

#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>

namespace nil::service::self
{
    struct Server::Impl
    {
        explicit Impl(const detail::Handlers& init_handlers)
            : handlers(init_handlers)
        {
        }

        void ready()
        {
            boost::asio::post(
                context,
                [this]()
                {
                    if (handlers.ready)
                    {
                        handlers.ready->call(id);
                    }
                    if (handlers.connect)
                    {
                        handlers.connect->call(id);
                    }
                }
            );
        }

        void run()
        {
            auto _ = boost::asio::make_work_guard(context);
            context.run();
        }

        void stop()
        {
            if (handlers.disconnect)
            {
                handlers.disconnect->call(id);
            }
            context.stop();
        }

        void publish(std::vector<std::uint8_t> data)
        {
            boost::asio::post(
                context,
                [this, msg = std::move(data)]()
                {
                    if (handlers.msg)
                    {
                        handlers.msg->call(id, msg.data(), msg.size());
                    }
                }
            );
        }

        const detail::Handlers& handlers;
        boost::asio::io_context context;

        ID id = {"self"};
    };

    Server::Server() = default;

    Server::~Server() noexcept = default;

    void Server::run()
    {
        if (!impl)
        {
            impl = std::make_unique<Impl>(handlers);
            impl->ready();
        }
        impl->run();
    }

    void Server::stop()
    {
        if (impl)
        {
            impl->stop();
        }
    }

    void Server::restart()
    {
        impl.reset();
    }

    void Server::publish(std::vector<std::uint8_t> data)
    {
        if (impl)
        {
            impl->publish(std::move(data));
        }
    }

    void Server::send(const ID& id, std::vector<std::uint8_t> data)
    {
        if (impl && "self" == id.text)
        {
            impl->publish(std::move(data));
        }
    }
}
