#pragma once

#include "ID.hpp"
#include "codec.hpp"
#include "detail.hpp"

#include <vector>

namespace nil::service
{
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

        template <typename T>
        void publish(const T& data)
        {
            this->publish(codec<T>::serialize(data));
        }

        template <typename T>
        void send(const ID& id, const T& data)
        {
            this->send(id, codec<T>::serialize(data));
        }

        /**
         * @brief Broadcast a message to all listeners
         *
         * @param type  message type
         * @param data  payload
         * @param size  payload size
         */
        void publish(const void* data, std::uint64_t size)
        {
            const auto* ptr = static_cast<const std::uint8_t*>(data);
            publish(std::vector<std::uint8_t>(ptr, ptr + size));
        }

        /**
         * @brief Send a message to a specific id.
         *  Does nothing if id is unknown.
         *
         * @param id    identifier
         * @param data  payload
         * @param size  payload size
         */
        void send(const ID& id, const void* data, std::uint64_t size)
        {
            const auto* ptr = static_cast<const std::uint8_t*>(data);
            send(id, std::vector<std::uint8_t>(ptr, ptr + size));
        }

        /**
         * @brief Add a message handler.
         *  Not threadsafe in case the service is already running.
         *
         * @param handler
         */
        template <typename Handler>
        void on_message(Handler handler)
        {
            handlers.msg = detail::create_message_handler(std::move(handler));
        }

        /**
         * @brief Add a connect handler for service events.
         *  Not threadsafe in case the service is already running.
         *
         * @param handler
         */
        template <typename Handler>
        void on_connect(Handler handler)
        {
            handlers.connect = detail::create_handler(std::move(handler));
        }

        /**
         * @brief Add a disconnect handler for service events.
         *  Not threadsafe in case the service is already running.
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
    };

    /**
     * @brief utility method to cast the payload into a specific type.
     *  expect data and size to move.
     *
     * @tparam T
     * @param data
     * @param size
     * @return T
     */
    template <typename T>
    inline T type_cast(const void*& data, std::uint64_t& size)
    {
        const auto o_size = size;
        T casted = detail::AutoCast(data, &size);
        data = static_cast<const std::uint8_t*>(data) + o_size - size;
        return casted;
    }
}
