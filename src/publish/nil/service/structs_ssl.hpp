#pragma once

#include "structs.hpp"

namespace nil::service
{
    struct HTTPSService;

    // HTTPSService Proxy
    // - returned by http::server::create
    // - owns the object and provides conversion mechanism to supported api
    struct S
    {
        std::unique_ptr<HTTPSService, void (*)(HTTPSService*)> ptr;

        operator HTTPSService&() const;    // NOLINT
        operator RunnableService&() const; // NOLINT
    };

    namespace impl
    {
        void on_ready(HTTPSService& service, std::function<void(const ID&)> handler);
    }

    template <typename Handler>
        requires(!std::is_same_v<void, decltype(detail::create_handler(std::declval<Handler>()))>)
    void on_ready(HTTPSService& service, Handler handler)
    {
        impl::on_ready(service, detail::create_handler(std::move(handler)));
    }

    P use_ws(HTTPSService& service, std::string route);
    struct HTTPSTransaction;
    void on_get(HTTPSService& service, std::function<void(const HTTPSTransaction&)> callback);
    std::string get_route(const HTTPSTransaction& transaction);
    void set_content_type(const HTTPSTransaction& transaction, std::string_view type);
    void send(const HTTPSTransaction& transaction, std::string_view body);
    void send(const HTTPSTransaction& transaction, const std::istream& body);
}
