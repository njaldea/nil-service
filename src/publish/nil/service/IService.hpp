#pragma once

#include "ID.hpp"
#include "codec.hpp"

#include <array>
#include <memory>
#include <type_traits>
#include <vector>

namespace nil::service
{
    namespace detail
    {
        template <typename... Args>
        struct ICallable
        {
            ICallable() = default;
            virtual ~ICallable() noexcept = default;

            ICallable(ICallable&&) noexcept = delete;
            ICallable& operator=(ICallable&&) noexcept = delete;

            ICallable(const ICallable&) = delete;
            ICallable& operator=(const ICallable&) = delete;

            virtual void call(Args... args) = 0;
        };

        template <typename T, typename... Args>
        struct Callable final: detail::ICallable<Args...>
        {
            explicit Callable(T init_impl)
                : impl(std::move(init_impl))
            {
            }

            ~Callable() noexcept override = default;

            Callable(Callable&&) noexcept = delete;
            Callable(const Callable&) = delete;
            Callable& operator=(Callable&&) noexcept = delete;
            Callable& operator=(const Callable&) = delete;

            void call(Args... args) override
            {
                impl(args...);
            }

            T impl;
        };

        struct Handlers
        {
            std::unique_ptr<ICallable<const ID&, const void*, std::uint64_t>> msg;
            std::unique_ptr<ICallable<const ID&>> connect;
            std::unique_ptr<ICallable<const ID&>> disconnect;
        };

        template <typename Handler>
        std::unique_ptr<ICallable<const ID&>> create_handler(Handler handler)
        {
            constexpr auto final_t = std::is_invocable_v<Handler, const ID&>;
            if constexpr (final_t)
            {
                using callable_t = detail::Callable<Handler, const ID&>;
                return std::make_unique<callable_t>(std::move(handler));
            }
            else
            {
                return create_handler([handler = std::move(handler)](const ID&) { handler(); });
            }
        }

        struct AutoCast
        {
            template <typename T>
            operator T() const // NOLINT
            {
                return codec<T>::deserialize(d, *s);
            }

            const void* d;
            std::uint64_t* s;
        };

        template <typename Handler>
        std::unique_ptr<ICallable<const ID&, const void*, std::uint64_t>> create_message_handler(
            Handler handler
        )
        {
            if constexpr (std::is_invocable_v<Handler, const ID&, const void*, std::uint64_t>)
            {
                using callable_t = detail::Callable<Handler, const ID&, const void*, std::uint64_t>;
                return std::make_unique<callable_t>(std::move(handler));
            }
            else if constexpr (std::is_invocable_v<Handler, const void*, std::uint64_t>)
            {
                return create_message_handler(                        //
                    [handler = std::move(handler)]                    //
                    (const ID&, const void* data, std::uint64_t size) //
                    { handler(data, size); }
                );
            }
            else if constexpr (std::is_invocable_v<Handler>)
            {
                return create_message_handler(              //
                    [handler = std::move(handler)]          //
                    (const ID&, const void*, std::uint64_t) //
                    { handler(); }
                );
            }
            else if constexpr (std::is_invocable_v<Handler, const nil::service::ID&>)
            {
                return create_message_handler(                 //
                    [handler = std::move(handler)]             //
                    (const ID& id, const void*, std::uint64_t) //
                    { handler(id); }
                );
            }
            else if constexpr (std::is_invocable_v<Handler, const ID&, const detail::AutoCast&>)
            {
                return create_message_handler(                           //
                    [handler = std::move(handler)]                       //
                    (const ID& id, const void* data, std::uint64_t size) //
                    { handler(id, AutoCast{data, &size}); }
                );
            }
            else
            {
                return create_message_handler(                        //
                    [handler = std::move(handler)]                    //
                    (const ID&, const void* data, std::uint64_t size) //
                    { handler(AutoCast{data, &size}); }
                );
            }
        }
    }

    class IService
    {
    public:
        virtual ~IService() noexcept = default;

        IService() = default;
        IService(IService&&) noexcept = delete;
        IService(const IService&) = delete;
        IService& operator=(IService&&) noexcept = delete;
        IService& operator=(const IService&) = delete;

        /**
         * @brief run the service. blocking.
         */
        virtual void run() = 0;

        /**
         * @brief stop the service. non-blocking.
         */
        virtual void stop() = 0;

        /**
         * @brief Prepare the service.
         *  Should be called once after stopping and before running.
         *  Call before calling other methods.
         */
        virtual void restart() = 0;

        /**
         * @brief Broadcast a message to all listeners
         *
         * @param type  message type
         * @param data  payload
         * @param size  payload size
         */
        virtual void publish(std::vector<std::uint8_t> message) = 0;

        /**
         * @brief Send a message to a specific id.
         *  Does nothing if id is unknown.
         *
         * @param id    identifier
         * @param data  data
         */
        virtual void send(const ID& id, std::vector<std::uint8_t> message) = 0;

        template <typename... T>
        void publish(const T&... data)
        {
            this->publish(serialize(data...));
        }

        /**
         * @brief Broadcast a message to all listeners
         *
         * @param type  message type
         * @param data  payload
         * @param size  payload size
         */
        void publish_raw(const void* data, std::uint64_t size)
        {
            const auto* ptr = static_cast<const std::uint8_t*>(data);
            publish(std::vector<std::uint8_t>(ptr, ptr + size));
        }

        template <typename... T>
        void send(const ID& id, T&&... data)
        {
            this->send(id, serialize(data...));
        }

        /**
         * @brief Send a message to a specific id.
         *  Does nothing if id is unknown.
         *
         * @param id    identifier
         * @param data  payload
         * @param size  payload size
         */
        void send_raw(const ID& id, const void* data, std::uint64_t size)
        {
            const auto* ptr = static_cast<const std::uint8_t*>(data);
            send(id, std::vector<std::uint8_t>(ptr, ptr + size));
        }

        /**
         * @brief Add a message handler. Not threadsafe in case the service is already running.
         *
         * @param handler
         */
        template <typename Handler>
        void on_message(Handler handler)
        {
            handlers.msg = detail::create_message_handler(std::move(handler));
        }

        /**
         * @brief Add a connect handler for service events. Not threadsafe in case the service is
         * already running.
         *
         * @param handler
         */
        template <typename Handler>
        void on_connect(Handler handler)
        {
            handlers.connect = detail::create_handler(std::move(handler));
        }

        /**
         * @brief Add a disconnect handler for service events. Not threadsafe in case the service is
         * already running.
         *
         * @param handler
         */
        template <typename Handler>
        void on_disconnect(Handler handler)
        {
            handlers.disconnect = detail::create_handler(std::move(handler));
        }

    protected:
        // NOLINTNEXTLINE(cppcoreguidelines-non-private-member-variables-in-classes)
        detail::Handlers handlers;

    private:
        // [TODO] refactor and make this better;
        template <typename... T>
        std::vector<std::uint8_t> serialize(const T&... data)
        {
            const std::array<std::vector<std::uint8_t>, sizeof...(T)> buffers
                = {codec<T>::serialize(data)...};

            std::vector<std::uint8_t> message;
            message.reserve(
                [&]()
                {
                    auto c = 0ul;
                    for (const auto& b : buffers)
                    {
                        c += b.size();
                    }
                    return c;
                }()
            );
            for (const auto& buffer : buffers)
            {
                for (const auto& item : buffer)
                {
                    message.push_back(item);
                }
            }
            return message;
        }
    };

    template <typename T>
    std::unique_ptr<IService> make_service(typename T::Options options)
    {
        return std::make_unique<T>(std::move(options));
    }
}
