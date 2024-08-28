#pragma once

#include "IService.hpp"

namespace nil::service
{
    class Self final: public IStandaloneService
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
        void send(const ID& id, std::vector<std::uint8_t> data) override;

        using IMessagingService::publish;
        using IMessagingService::send;

        using IObservableService::on_connect;
        using IObservableService::on_disconnect;
        using IObservableService::on_message;
        using IObservableService::on_ready;

    private:
        struct Impl;
        std::unique_ptr<Impl> impl;
    };
}
