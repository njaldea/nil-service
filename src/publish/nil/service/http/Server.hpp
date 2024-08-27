#pragma once

#include "../detail/Callable.hpp"
#include "../detail/create_handler.hpp"

#include <cstdint>
#include <memory>

namespace nil::service::http
{
    class Server final
    {
    public:
        struct Options final
        {
            std::uint16_t port;
            std::uint64_t buffer = 8192;
        };

        explicit Server(Options init_options);

        /**
         * @note make sure the service is not running before destroying it.
         */
        ~Server() noexcept;

        Server(Server&&) noexcept = delete;
        Server& operator=(Server&&) noexcept = delete;
        Server(const Server&) = delete;
        Server& operator=(const Server&) = delete;

        void run();
        void stop();
        void restart();

        template <typename T>
        void use(const std::string& route, const std::string& content_type, T body)
        {
            use_impl(
                route,
                content_type,
                std::make_unique<detail::Callable<T, std::ostream&>>(std::move(body))
            );
        }

        template <typename Handler>
        void on_ready(Handler handler)
        {
            ready = detail::create_handler(std::move(handler));
        }

    private:
        void use_impl(
            std::string route,
            std::string content_type,
            std::unique_ptr<detail::ICallable<std::ostream&>> body
        );

        Options options;
        std::unique_ptr<detail::ICallable<const ID&>> ready;

        struct Routes;
        std::unique_ptr<Routes> routes;

        struct Impl;
        std::unique_ptr<Impl> impl;
    };
}
