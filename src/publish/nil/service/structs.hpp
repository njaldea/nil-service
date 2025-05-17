#pragma once

#include "concat.hpp"
#include "detail/create_handler.hpp"
#include "detail/create_message_handler.hpp"

#include <memory>
#include <type_traits>

namespace nil::service
{
    struct StandaloneService;
    struct WebService;
    struct Service;

    struct RunnableService;
    struct MessagingService;
    struct ObservableService;

    // Service Proxy
    // - returned by use_ws when a route is added to handle web socket connection.
    // - does not own the object and provides conversion mechanism to supported api
    // - the object is owned by the parent service (HTTPService/H)
    struct P
    {
        Service* ptr;
        operator Service&() const;           // NOLINT
        operator MessagingService&() const;  // NOLINT
        operator ObservableService&() const; // NOLINT
    };

    // StandaloneService Proxy
    // - mainly returned by create methods
    // - owns the object and provides conversion mechanism to supported api
    struct A
    {
        std::unique_ptr<StandaloneService, void (*)(StandaloneService*)> ptr;

        operator StandaloneService&() const; // NOLINT
        operator Service&() const;           // NOLINT
        operator RunnableService&() const;   // NOLINT
        operator MessagingService&() const;  // NOLINT
        operator ObservableService&() const; // NOLINT
        operator P() const;                  // NOLINT
    };

    // WebService Proxy
    // - returned by http::server::create
    // - owns the object and provides conversion mechanism to supported api
    struct W
    {
        std::unique_ptr<WebService, void (*)(WebService*)> ptr;

        operator WebService&() const;      // NOLINT
        operator RunnableService&() const; // NOLINT
    };

    namespace impl
    {
        void on_ready(ObservableService& service, std::function<void(const ID&)> handler);
        void on_connect(ObservableService& service, std::function<void(const ID&)> handler);
        void on_disconnect(ObservableService& service, std::function<void(const ID&)> handler);
        void on_message(
            ObservableService& service,
            std::function<void(const ID&, const void*, std::uint64_t)> handler
        );
    }

    /**
     * @brief start the service. blocking.
     */
    void start(RunnableService& service);

    /**
     * @brief stop the service. non-blocking.
     */
    void stop(RunnableService& service);

    /**
     * @brief Prepare the service.
     *  Should be called once after stopping and before running.
     *  Call before calling other methods.
     */
    void restart(RunnableService& service);

    // clang-format off
    void publish(MessagingService& service, std::vector<std::uint8_t> payload);
    void publish_ex(MessagingService& service, ID id, std::vector<std::uint8_t> payload);
    void send(MessagingService& service, ID id, std::vector<std::uint8_t> payload);
    void send(MessagingService& service, std::vector<ID> ids, std::vector<std::uint8_t> payload);

    void publish(MessagingService& service, const void* data, std::uint64_t size);
    void publish_ex(MessagingService& service, const ID& id, const void* data, std::uint64_t size);
    void send(MessagingService& service, ID id, const void* data, std::uint64_t size);
    void send(MessagingService& service, std::vector<ID> ids, const void* data, std::uint64_t size);
    // clang-format on

    template <typename T>
        requires(!std::is_same_v<std::vector<std::uint8_t>, T>)
    void publish(MessagingService& service, const T& data)
    {
        publish(service, concat(data));
    }

    template <typename T>
        requires(!std::is_same_v<std::vector<std::uint8_t>, T>)
    void send(MessagingService& service, ID id, const T& data)
    {
        send(service, std::move(id), concat(data));
    }

    template <typename T>
        requires(!std::is_same_v<std::vector<std::uint8_t>, T>)
    void send(MessagingService& service, std::vector<ID> ids, const T& data)
    {
        send(service, std::move(ids), concat(data));
    }

    /**
     * @brief Add ready handler for service events.
     *  Not threadsafe in case the service is already running.
     *
     * @param handler
     */
    template <typename T>
        requires(!std::is_same_v<void, decltype(detail::create_handler(std::declval<T>()))>)
    void on_ready(ObservableService& service, T handler)
    {
        impl::on_ready(service, detail::create_handler(std::move(handler)));
    }

    /**
     * @brief Add a connect handler for service events.
     *  Not threadsafe in case the service is already running.
     *
     * @param handler
     */
    template <typename T>
        requires(!std::is_same_v<void, decltype(detail::create_handler(std::declval<T>()))>)
    void on_connect(ObservableService& service, T handler)
    {
        impl::on_connect(service, detail::create_handler(std::move(handler)));
    }

    /**
     * @brief Add a disconnect handler for service events.
     *  Not threadsafe in case the service is already running.
     *
     * @param handler
     */
    template <typename T>
        requires(!std::is_same_v<void, decltype(detail::create_handler(std::declval<T>()))>)
    void on_disconnect(ObservableService& service, T handler)
    {
        impl::on_disconnect(service, detail::create_handler(std::move(handler)));
    }

    /**
     * @brief Add a message handler.
     *  Not threadsafe in case the service is already running.
     *
     * @param handler
     */
    template <typename T>
        requires(!std::is_same_v<void, decltype(detail::create_message_handler(std::declval<T>()))>)
    void on_message(ObservableService& service, T handler)
    {
        impl::on_message(service, detail::create_message_handler(std::move(handler)));
    }

    namespace impl
    {
        void on_ready(WebService& service, std::function<void(const ID&)> handler);
    }

    /**
     * @brief Add ready handler for service events.
     *  Not threadsafe in case the service is already running.
     *
     * @param handler
     */
    template <typename Handler>
        requires(!std::is_same_v<void, decltype(detail::create_handler(std::declval<Handler>()))>)
    void on_ready(WebService& service, Handler handler)
    {
        impl::on_ready(service, detail::create_handler(std::move(handler)));
    }

    P use_ws(WebService& service, const std::string& route);
    struct WebTransaction;
    void on_get(WebService& service, std::function<void(const WebTransaction&)> callback);
    std::string_view get_route(const WebTransaction& transaction);
    void set_content_type(const WebTransaction& transaction, std::string_view type);
    void send(const WebTransaction& transaction, std::string_view body);
    void send(const WebTransaction& transaction, const std::istream& body);
}
