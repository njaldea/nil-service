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
        void on_ready(
            ObservableService& service,
            std::unique_ptr<detail::ICallable<const ID&>> handler
        );
        void on_connect(
            ObservableService& service,
            std::unique_ptr<detail::ICallable<const ID&>> handler
        );
        void on_disconnect(
            ObservableService& service,
            std::unique_ptr<detail::ICallable<const ID&>> handler
        );
        void on_message(
            ObservableService& service,
            std::unique_ptr<detail::ICallable<const ID&, const void*, std::uint64_t>> handler
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

    /**
     * @brief Broadcast a message to all listeners
     *
     * @param type  message type
     * @param data  payload
     * @param size  payload size
     */
    void publish(MessagingService& service, std::vector<std::uint8_t> payload);

    /**
     * @brief Send a message to a specific id.
     *  Does nothing if id is unknown.
     *
     * @param id    identifier
     * @param data  data
     */
    void send(MessagingService& service, const ID& id, std::vector<std::uint8_t> payload);

    /**
     * @brief Broadcast a message to all listeners
     *
     * @param type  message type
     * @param data  payload
     * @param size  payload size
     */
    void publish(MessagingService& service, const void* data, std::uint64_t size);

    /**
     * @brief Send a message to a specific id.
     *  Does nothing if id is unknown.
     *
     * @param id    identifier
     * @param data  payload
     * @param size  payload size
     */
    void send(MessagingService& service, const ID& id, const void* data, std::uint64_t size);

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

    /**
     * @brief Add ready handler for service events.
     *  Not threadsafe in case the service is already running.
     *
     * @param handler
     */
    template <typename T>
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
    void on_message(ObservableService& service, T handler)
    {
        impl::on_message(service, detail::create_message_handler(std::move(handler)));
    }

    namespace impl
    {
        void use(
            HTTPService& service,
            std::string route,
            std::string content_type,
            std::unique_ptr<detail::ICallable<std::ostream&>> body
        );
        void on_ready(HTTPService& service, std::unique_ptr<detail::ICallable<const ID&>> handler);
    }

    template <typename T>
    void use(
        HTTPService& service,
        const std::string& route,
        const std::string& content_type,
        T body
    )
    {
        impl::use(
            service,
            route,
            content_type,
            std::make_unique<detail::Callable<T, std::ostream&>>(std::move(body))
        );
    }

    /**
     * @brief Add a websocket server for a specific route
     *
     * @param service
     * @param route
     * @return S
     */
    S use_ws(HTTPService& service, std::string route);

    /**
     * @brief Add ready handler for service events.
     *  Not threadsafe in case the service is already running.
     *
     * @param handler
     */
    template <typename Handler>
    void on_ready(HTTPService& service, Handler handler)
    {
        impl::on_ready(service, detail::create_handler(std::move(handler)));
    }
}
