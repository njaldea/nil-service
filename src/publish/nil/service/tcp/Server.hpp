#pragma once

#include "../IService.hpp"

#include <memory>

namespace nil::service::tcp
{
    class Server final: public IService
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

        using IService::on_connect;
        using IService::on_disconnect;
        using IService::on_message;
        using IService::on_ready;
        using IService::publish;
        using IService::send;

    private:
        Options options;

        struct Impl;
        std::unique_ptr<Impl> impl;
    };
}
