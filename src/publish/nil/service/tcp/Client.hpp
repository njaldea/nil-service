#pragma once

#include "../IService.hpp"

#include <memory>

namespace nil::service::tcp
{
    class Client final: public IService
    {
    public:
        struct Options final
        {
            std::string host;
            std::uint16_t port;
            /**
             * @brief buffer size to use:
             *  - one for receiving
             */
            std::uint64_t buffer = 1024;
        };

        explicit Client(Options init_options);
        ~Client() noexcept override;

        Client(Client&&) noexcept = delete;
        Client& operator=(Client&&) noexcept = delete;

        Client(const Client&) = delete;
        Client& operator=(const Client&) = delete;

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
