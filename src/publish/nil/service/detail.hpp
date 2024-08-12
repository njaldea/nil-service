#pragma once

#include "ID.hpp"
#include "codec.hpp"

#include <memory>
#include <utility>

namespace nil::service::detail
{
    /**
     * @brief used to cause compilation error for cases that uses if constexpr
     *
     * @tparam T
     */
    template <typename T>
    void unreachable() = delete;

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

    struct Handlers
    {
        std::unique_ptr<ICallable<const ID&, const void*, std::uint64_t>> msg;
        std::unique_ptr<ICallable<const ID&>> connect;
        std::unique_ptr<ICallable<const ID&>> disconnect;
    };

    /**
     * @brief adapter method so that the handler can be converted to appropriate type.
     *  Handler is expected to have the following signature:
     *   -  void method(const ID&)
     *   -  void method()
     *
     * @tparam Handler
     * @param handler
     * @return std::unique_ptr<ICallable<const ID&>>
     */
    template <typename Handler>
    std::unique_ptr<ICallable<const ID&>> create_handler(Handler handler)
    {
        if constexpr (std::is_invocable_v<Handler, const ID&>)
        {
            using callable_t = detail::Callable<Handler, const ID&>;
            return std::make_unique<callable_t>(std::move(handler));
        }
        else if constexpr (std::is_invocable_v<Handler>)
        {
            return create_handler([handler = std::move(handler)](const ID&) { handler(); });
        }
        else
        {
            unreachable<Handler>();
        }
    }

    /**
     * @brief a hack to bypass argument type detection of the handlers.
     *  this is only used internally to avoid convoluted templates just to detect the type.
     *
     * @tparam Args
     */
    struct AutoCast
    {
        template <typename T>
        operator T() const // NOLINT
        {
            return codec<T>::deserialize(d, *s);
        }

        const void* d;
        std::uint64_t* s;
    };

    /**
     * @brief adapter method so that the handler can be converted to appropriate type.
     *  Handler is expected to have the following signature:
     *   -  void method(const ID&, const void*, std::uint64_t)
     *   -  void method(const void*, std::uint64_t)
     *   -  void method()
     *   -  void method(const ID&)
     *   -  void method(const ID&, const WithCodec&)
     *   -  void method(const WithCodec&)
     *
     * @tparam Handler
     * @param handler
     * @return std::unique_ptr<ICallable<const ID&, const void*, std::uint64_t>>
     */
    template <typename Handler>
    std::unique_ptr<ICallable<const ID&, const void*, std::uint64_t>> create_message_handler(
        Handler handler
    )
    {
        if constexpr (std::is_invocable_v<Handler, const ID&, const void*, std::uint64_t>)
        {
            using callable_t = Callable<Handler, const ID&, const void*, std::uint64_t>;
            return std::make_unique<callable_t>(std::move(handler));
        }
        else if constexpr (std::is_invocable_v<Handler, const void*, std::uint64_t>)
        {
            return create_message_handler(                        //
                [handler = std::move(handler)]                    //
                (const ID&, const void* data, std::uint64_t size) //
                { handler(data, size); }
            );
        }
        else if constexpr (std::is_invocable_v<Handler, const nil::service::ID&>)
        {
            return create_message_handler(                 //
                [handler = std::move(handler)]             //
                (const ID& id, const void*, std::uint64_t) //
                { handler(id); }
            );
        }
        else if constexpr (std::is_invocable_v<Handler, const ID&, const AutoCast&>)
        {
            return create_message_handler(                           //
                [handler = std::move(handler)]                       //
                (const ID& id, const void* data, std::uint64_t size) //
                { handler(id, AutoCast{data, &size}); }
            );
        }
        else if constexpr (std::is_invocable_v<Handler, const AutoCast&>)
        {
            return create_message_handler(                        //
                [handler = std::move(handler)]                    //
                (const ID&, const void* data, std::uint64_t size) //
                { handler(AutoCast{data, &size}); }
            );
        }
        else if constexpr (std::is_invocable_v<Handler>)
        {
            return create_message_handler(              //
                [handler = std::move(handler)]          //
                (const ID&, const void*, std::uint64_t) //
                { handler(); }
            );
        }
        else
        {
            unreachable<Handler>();
        }
    }
}
