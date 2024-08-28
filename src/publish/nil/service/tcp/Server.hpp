#pragma once

#include "../IService.hpp"

#include <memory>

namespace nil::service::tcp
{
    class Server final: public IStandaloneService
    {
    public:
        struct Options final
        {
            std::uint16_t port;
            /**
             * @brief buffer size to use:
             *  - one for receiving per connection
             */
            std::uint64_t buffer = 1024;
        };

        explicit Server(Options init_options);
        ~Server() noexcept override;

        Server(Server&&) noexcept = delete;
        Server& operator=(Server&&) noexcept = delete;

        Server(const Server&) = delete;
        Server& operator=(const Server&) = delete;

        void run() override;
        void stop() override;
        void restart() override;

        void publish(std::vector<std::uint8_t> data) override;
        void send(const ID& id, std::vector<std::uint8_t> data) override;

        using IMessagingService::publish;
        using IMessagingService::send;

        using IObservableService::on_connect;
        using IObservableService::on_disconnect;
        using IObservableService::on_message;
        using IObservableService::on_ready;

    private:
        Options options;

        struct Impl;
        std::unique_ptr<Impl> impl;
    };
}
