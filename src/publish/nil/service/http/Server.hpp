#pragma once

#include "../detail/Callable.hpp"
#include "nil/service/IService.hpp"

#include <cstdint>
#include <memory>

namespace nil::service::http
{
    class WebSocket final: private IService
    {
        friend class Server;

    public:
        struct Impl;
        std::unique_ptr<Impl> impl;

        explicit WebSocket(std::unique_ptr<Impl> init_impl);
        ~WebSocket() noexcept override;
        WebSocket(WebSocket&&) = delete;
        WebSocket(const WebSocket&) = delete;
        WebSocket& operator=(WebSocket&&) = delete;
        WebSocket& operator=(const WebSocket&) = delete;

        void publish(std::vector<std::uint8_t> data) override;
        void send(const ID& id, std::vector<std::uint8_t> data) override;

        using IService::on_connect;
        using IService::on_disconnect;
        using IService::on_message;
        using IService::on_ready;
        using IService::publish;
        using IService::send;

    private:
        void run() override;
        void stop() override;
        void restart() override;
    };

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

        WebSocket& use_ws(std::string route);

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

        struct State;
        std::unique_ptr<State> state;

        struct Impl;
        std::unique_ptr<Impl> impl;
    };
}
