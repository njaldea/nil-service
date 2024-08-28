#include <nil/service/Self.hpp>

#include <boost/asio/executor_work_guard.hpp>

#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>

namespace nil::service
{
    struct Self::Impl
    {
        explicit Impl(const detail::Handlers& init_handlers)
            : handlers(init_handlers)
        {
        }

        void run()
        {
            auto _ = boost::asio::make_work_guard(context);
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

    Self::Self() = default;

    Self::~Self() noexcept = default;

    void Self::run()
    {
        impl = std::make_unique<Impl>(handlers);
        impl->run();
    }

    void Self::stop()
    {
        if (impl)
        {
            impl->stop();
        }
    }

    void Self::restart()
    {
        impl.reset();
    }

    void Self::publish(std::vector<std::uint8_t> data)
    {
        if (impl)
        {
            impl->publish(std::move(data));
        }
    }

    void Self::send(const ID& id, std::vector<std::uint8_t> data)
    {
        if (impl && "self" == id.text)
        {
            impl->publish(std::move(data));
        }
    }
}
