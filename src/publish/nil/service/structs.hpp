#pragma once

#include "concat.hpp"
#include "detail/create_handler.hpp"
#include "detail/create_message_handler.hpp"

#include <cstdint>
#include <functional>
#include <iosfwd>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace nil::service
{
    struct IRunnableService
    {
        IRunnableService() = default;
        virtual ~IRunnableService() noexcept = default;
        IRunnableService(const IRunnableService&) = delete;
        IRunnableService(IRunnableService&&) = delete;
        IRunnableService& operator=(const IRunnableService&) = delete;
        IRunnableService& operator=(IRunnableService&&) = delete;

        /**
         * @brief start the service. blocking.
         */
        virtual void start() = 0;

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
         * @brief execute the task in the service thread
         */
        virtual void dispatch(std::function<void()> task) = 0;
    };

    struct IMessageService
    {
        IMessageService() = default;
        virtual ~IMessageService() noexcept = default;
        IMessageService(const IMessageService&) = delete;
        IMessageService(IMessageService&&) = delete;
        IMessageService& operator=(const IMessageService&) = delete;
        IMessageService& operator=(IMessageService&&) = delete;

        virtual void publish(std::vector<std::uint8_t> payload) = 0;
        virtual void publish_ex(std::vector<ID> ids, std::vector<std::uint8_t> payload) = 0;
        virtual void send(std::vector<ID> ids, std::vector<std::uint8_t> payload) = 0;

        void publish(const void* data, std::uint64_t size)
        {
            const auto* ptr = static_cast<const std::uint8_t*>(data);
            publish(std::vector<std::uint8_t>(ptr, ptr + size));
        }

        void publish_ex(ID id, const void* data, std::uint64_t size)
        {
            const auto* ptr = static_cast<const std::uint8_t*>(data);
            publish_ex(std::vector<ID>{id}, std::vector<std::uint8_t>(ptr, ptr + size));
        }

        void send(ID id, const void* data, std::uint64_t size)
        {
            const auto* ptr = static_cast<const std::uint8_t*>(data);
            send(std::vector<ID>{id}, std::vector<std::uint8_t>(ptr, ptr + size));
        }

        template <typename T>
            requires(!std::is_same_v<std::vector<std::uint8_t>, T>)
        void publish(const T& data)
        {
            publish(concat(data));
        }

        template <typename T>
            requires(!std::is_same_v<std::vector<std::uint8_t>, T>)
        void send(ID id, const T& data)
        {
            send(std::vector<ID>{id}, concat(data));
        }

        template <typename T>
            requires(!std::is_same_v<std::vector<std::uint8_t>, T>)
        void send(std::vector<ID> ids, const T& data)
        {
            send(std::move(ids), concat(data));
        }

        void send(ID id, std::vector<std::uint8_t> payload)
        {
            send(std::vector<ID>{id}, std::move(payload));
        }
    };

    struct ICallbackService
    {
        ICallbackService() = default;
        virtual ~ICallbackService() noexcept = default;
        ICallbackService(const ICallbackService&) = delete;
        ICallbackService(ICallbackService&&) = delete;
        ICallbackService& operator=(const ICallbackService&) = delete;
        ICallbackService& operator=(ICallbackService&&) = delete;

        /**
         * @brief Add ready handler for service events.
         *  Not threadsafe in case the service is already running.
         *
         * @param handler
         */
        template <typename T>
            requires(!std::is_same_v<void, decltype(detail::create_handler(std::declval<T>()))>)
        void on_ready(T handler)
        {
            impl_on_ready(detail::create_handler(std::move(handler)));
        }

        /**
         * @brief Add a connect handler for service events.
         *  Not threadsafe in case the service is already running.
         *
         * @param handler
         */
        template <typename T>
            requires(!std::is_same_v<void, decltype(detail::create_handler(std::declval<T>()))>)
        void on_connect(T handler)
        {
            impl_on_connect(detail::create_handler(std::move(handler)));
        }

        /**
         * @brief Add a disconnect handler for service events.
         *  Not threadsafe in case the service is already running.
         *
         * @param handler
         */
        template <typename T>
            requires(!std::is_same_v<void, decltype(detail::create_handler(std::declval<T>()))>)
        void on_disconnect(T handler)
        {
            impl_on_disconnect(detail::create_handler(std::move(handler)));
        }

        /**
         * @brief Add a message handler.
         *  Not threadsafe in case the service is already running.
         *
         * @param handler
         */
        template <typename T>
            requires(!std::is_same_v<
                     void,
                     decltype(detail::create_message_handler(std::declval<T>()))>)
        void on_message(T handler)
        {
            impl_on_message(detail::create_message_handler(std::move(handler)));
        }

    private:
        virtual void impl_on_message(std::function<void(ID, const void*, std::uint64_t)> handler)
            = 0;
        virtual void impl_on_ready(std::function<void(ID)> handler) = 0;
        virtual void impl_on_connect(std::function<void(ID)> handler) = 0;
        virtual void impl_on_disconnect(std::function<void(ID)> handler) = 0;
    };

    struct IEventService
        : IMessageService
        , ICallbackService
    {
    };

    struct WebTransaction;

    struct IWebService: IRunnableService
    {
        void on_get(std::function<bool(WebTransaction&)> handler)
        {
            impl_on_get(std::move(handler));
        }

        void on_ready(std::function<void(ID)> handler)
        {
            impl_on_ready(std::move(handler));
        }

        virtual IEventService* use_ws(const std::string& key) = 0;

        /**
         * @brief Add ready handler for service events.
         *  Not threadsafe in case the service is already running.
         *
         * @param handler
         */
        template <typename Handler>
            requires(!std::
                         is_same_v<void, decltype(detail::create_handler(std::declval<Handler>()))>)
        void on_ready(Handler handler)
        {
            impl_on_ready(detail::create_handler(std::move(handler)));
        }

    private:
        virtual void impl_on_get(std::function<bool(WebTransaction&)> callback) = 0;
        virtual void impl_on_ready(std::function<void(ID)> handler) = 0;
    };

    void set_content_type(WebTransaction& transaction, std::string_view type);
    std::string_view get_route(const WebTransaction& transaction);
    void send(const WebTransaction& transaction, std::string_view body);
    void send(const WebTransaction& transaction, const std::istream& body);

    struct IStandaloneService
        : IEventService
        , IRunnableService
    {
    };

    struct IGatewayService: IStandaloneService
    {
        IGatewayService() = default;
        ~IGatewayService() noexcept override = default;
        IGatewayService(const IGatewayService&) = delete;
        IGatewayService(IGatewayService&&) = delete;
        IGatewayService& operator=(const IGatewayService&) = delete;
        IGatewayService& operator=(IGatewayService&&) = delete;
        virtual void add_service(IEventService& service) = 0;
    };
}
