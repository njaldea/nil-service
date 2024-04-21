#pragma once

#include "IService.hpp"

namespace nil::service
{
    class Self: public IService
    {
    public:
        Self();
        ~Self() noexcept override;

        Self(Self&&) noexcept = delete;
        Self& operator=(Self&&) noexcept = delete;

        Self(const Self&) = delete;
        Self& operator=(const Self&) = delete;

        void run() override;
        void stop() override;
        void restart() override;

        void publish(std::vector<std::uint8_t> data) override;
        void send(const std::string& id, std::vector<std::uint8_t> data) override;

        using IService::publish;
        using IService::publish_raw;
        using IService::send;
        using IService::send_raw;

    private:
        struct Storage
        {
            std::unique_ptr<detail::ICallable<const void*, std::uint64_t>> msg;
            std::unique_ptr<detail::ICallable<>> connect;
            std::unique_ptr<detail::ICallable<>> disconnect;
        };

        Storage storage;

        struct Impl;
        std::unique_ptr<Impl> impl;

        void on_message_impl(MessageHandler handler) override;
        void on_connect_impl(ConnectHandler handler) override;
        void on_disconnect_impl(DisconnectHandler handler) override;
    };
}
