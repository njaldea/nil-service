#pragma once

#include <utility>

namespace nil::service::detail
{
    /**
     * @brief just alternative to std::function
     *
     * @tparam Args
     */
    template <typename... Args>
    struct ICallable
    {
        ICallable() = default;
        virtual ~ICallable() noexcept = default;

        ICallable(ICallable&&) noexcept = delete;
        ICallable& operator=(ICallable&&) noexcept = delete;

        ICallable(const ICallable&) = delete;
        ICallable& operator=(const ICallable&) = delete;

        virtual void call(Args... args) = 0;
    };

    /**
     * @brief just alternative to std::function
     *
     * @tparam Args
     */
    template <typename T, typename... Args>
    struct Callable final: detail::ICallable<Args...>
    {
        explicit Callable(T init_impl)
            : impl(std::move(init_impl))
        {
        }

        ~Callable() noexcept override = default;

        Callable(Callable&&) noexcept = delete;
        Callable(const Callable&) = delete;
        Callable& operator=(Callable&&) noexcept = delete;
        Callable& operator=(const Callable&) = delete;

        void call(Args... args) override
        {
            impl(args...);
        }

        T impl;
    };
}
