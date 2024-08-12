#pragma once

#include "IService.hpp"

namespace nil::service
{
    class Self final: public IService
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

        using IService::publish;
        using IService::send;

    private:
        struct Impl;
        std::unique_ptr<Impl> impl;
    };
}
