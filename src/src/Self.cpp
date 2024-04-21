#include <boost/asio/executor_work_guard.hpp>
#include <nil/service/Self.hpp>

#include <boost/asio/dispatch.hpp>
#include <boost/asio/io_context.hpp>

namespace nil::service
{
    struct Self::Impl
    {
        explicit Impl(const Self::Storage& init_storage)
            : storage(init_storage)
        {
        }

        void run()
        {
            auto _ = boost::asio::make_work_guard(context);
            boost::asio::dispatch(
                context,
                [this]()
                {
                    if (storage.connect)
                    {
                        storage.connect->call("self");
                    }
                }
            );
            context.run();
        }

        void stop()
        {
            if (storage.disconnect)
            {
                storage.disconnect->call("self");
            }
            context.stop();
        }

        void publish(std::vector<std::uint8_t> data)
        {
            boost::asio::dispatch(
                context,
                [this, msg = std::move(data)]()
                {
                    if (storage.msg)
                    {
                        storage.msg->call("self", msg.data(), msg.size());
                    }
                }
            );
        }

        const Self::Storage& storage;
        boost::asio::io_context context;
    };

    Self::Self()
        : impl(std::make_unique<Impl>(storage))
    {
    }

    Self::~Self() noexcept = default;

    void Self::run()
    {
        impl->run();
    }

    void Self::stop()
    {
        impl->stop();
    }

    void Self::restart()
    {
        impl.reset();
        impl = std::make_unique<Impl>(storage);
    }

    void Self::publish(std::vector<std::uint8_t> data)
    {
        impl->publish(std::move(data));
    }

    void Self::send(const std::string& id, std::vector<std::uint8_t> data)
    {
        if ("self" == id)
        {
            impl->publish(std::move(data));
        }
    }

    void Self::on_message_impl(MessageHandler handler)
    {
        storage.msg = std::move(handler);
    }

    void Self::on_connect_impl(ConnectHandler handler)
    {
        storage.connect = std::move(handler);
    }

    void Self::on_disconnect_impl(DisconnectHandler handler)
    {
        storage.disconnect = std::move(handler);
    }
}
