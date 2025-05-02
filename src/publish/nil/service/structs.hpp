#pragma once

#include "codec.hpp"
#include "detail/create_handler.hpp"
#include "detail/create_message_handler.hpp"

#include <memory>
#include <type_traits>

namespace nil::service
{
    struct StandaloneService;
    struct HTTPService;
    struct Service;

    struct RunnableService;
    struct MessagingService;
    struct ObservableService;

    // Service Proxy
    // - returned by use_ws when a route is added to handle web socket connection.
    // - does not own the object and provides conversion mechanism to supported api
    // - the object is owned by the parent service (HTTPService/H)
    struct S
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
        operator S() const;                  // NOLINT
    };

    // HTTPService Proxy
    // - returned by http::server::create
    // - owns the object and provides conversion mechanism to supported api
    struct H
    {
        std::unique_ptr<HTTPService, void (*)(HTTPService*)> ptr;

        operator HTTPService&() const;     // NOLINT
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
    void publish_ex(MessagingService& service, const ID& id, std::vector<std::uint8_t> payload);
    void send(MessagingService& service, const ID& id, std::vector<std::uint8_t> payload);
    void send(MessagingService& service, const std::vector<ID>& ids, std::vector<std::uint8_t> payload);

    void publish(MessagingService& service, const void* data, std::uint64_t size);
    void publish_ex(MessagingService& service, const ID& id, const void* data, std::uint64_t size);
    void send(MessagingService& service, const ID& id, const void* data, std::uint64_t size);
    void send(MessagingService& service, const std::vector<ID>& ids, const void* data, std::uint64_t size);
    // clang-format on

    template <typename T>
        requires(!std::is_same_v<std::vector<std::uint8_t>, T>)
    void publish(MessagingService& service, const T& data)
    {
        publish(service, codec<T>::serialize(data));
    }

    template <typename T>
        requires(!std::is_same_v<std::vector<std::uint8_t>, T>)
    void send(MessagingService& service, const ID& id, const T& data)
    {
        send(service, id, codec<T>::serialize(data));
    }

    template <typename T>
        requires(!std::is_same_v<std::vector<std::uint8_t>, T>)
    void send(MessagingService& service, const std::vector<ID>& ids, const T& data)
    {
        send(service, ids, codec<T>::serialize(data));
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
        void on_ready(HTTPService& service, std::function<void(const ID&)> handler);
    }

    /**
     * @brief Add ready handler for service events.
     *  Not threadsafe in case the service is already running.
     *
     * @param handler
     */
    template <typename Handler>
        requires(!std::is_same_v<void, decltype(detail::create_handler(std::declval<Handler>()))>)
    void on_ready(HTTPService& service, Handler handler)
    {
        impl::on_ready(service, detail::create_handler(std::move(handler)));
    }

    /**
     * @brief Add a websocket server for a specific route
     *
     * @param service
     * @param route
     * @return S
     */
    S use_ws(HTTPService& service, std::string route);

    struct HTTPTransaction;

    void on_get(HTTPService& service, std::function<void(const HTTPTransaction&)> callback);

    std::string get_route(const HTTPTransaction& transaction);
    void set_content_type(const HTTPTransaction& transaction, std::string_view type);
    void send(const HTTPTransaction& transaction, std::string_view body);
    void send(const HTTPTransaction& transaction, const std::istream& body);
}
