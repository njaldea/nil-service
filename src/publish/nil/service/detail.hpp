#pragma once

#include "ID.hpp"
#include "codec.hpp"

#include <memory>
#include <utility>

namespace nil::service::detail
{
    /**
     * @brief used to cause compilation error when argument types of a callable is invalid
     *
     * @tparam T
     */
    template <typename T, typename... Rest>
    void argument_error(Rest...) = delete;

    /**
     * @brief used to cause compilation error in case when there is a missing branch
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
        constexpr auto match                          //
            = std::is_invocable_v<Handler, const ID&> //
            + std::is_invocable_v<Handler>;
        if constexpr (0 == match)
        {
            argument_error<Handler>("incorrect argument type detected");
        }
        else if constexpr (1 < match)
        {
            argument_error<Handler>("ambiguous argument type detected");
        }
        else if constexpr (std::is_invocable_v<Handler, const ID&>)
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
     */
    struct AutoCast final
    {
        AutoCast(const void* init_d, std::uint64_t* init_s)
            : d(init_d)
            , s(init_s)
        {
        }

        ~AutoCast() = default;

        AutoCast(const AutoCast&) = delete;
        AutoCast(AutoCast&&) = delete;
        AutoCast& operator=(const AutoCast&) = delete;
        AutoCast& operator=(AutoCast&&) = delete;

        template <typename T>
        operator T() const // NOLINT
        {
            return codec<T>::deserialize(d, *s);
        }

        operator ID() const = delete;

        const void* d;
        std::uint64_t* s;
    };

    /**
     * @brief This is used to detect if the user provided a callable with `const auto&` argument.
     *  If const auto& is provided, the AutoCast passed by the library will not trigger implicit
     * conversion. This means that in the body of the callable, data is not yet consumed which will
     * cause weird behavior.
     */
    struct Unknown
    {
    };

    /**
     * @brief adapter method so that the handler can be converted to appropriate type.
     *  Handler is expected to have the following signature:
     *   -  void method()
     *   -  void method(const ID&)
     *   -  void method(const ID&, const WithCodec&)
     *   -  void method(const ID&, const void*, std::uint64_t)
     *   -  void method(const WithCodec&)
     *   -  void method(const void*, std::uint64_t)
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
        constexpr auto match               //
            = std::is_invocable_v<Handler> //
            + std::is_invocable_v<Handler, const ID&>
            + std::is_invocable_v<Handler, const ID&, const AutoCast&>
            + std::is_invocable_v<Handler, const ID&, const void*, std::uint64_t>
            + std::is_invocable_v<Handler, const AutoCast&>
            + std::is_invocable_v<Handler, const void*, std::uint64_t>;

        constexpr auto has_unknown_type //
            = std::is_invocable_v<Handler, const ID&, const Unknown&>
            + std::is_invocable_v<Handler, const Unknown&>;

        if constexpr (0 == match)
        {
            argument_error<Handler>("incorrect argument type detected");
        }
        else if constexpr (1 < match)
        {
            argument_error<Handler>("ambiguous argument type detected");
        }
        else if constexpr (0 < has_unknown_type)
        {
            argument_error<Handler>("unknown argument type detected");
        }
        else if constexpr (std::is_invocable_v<Handler>)
        {
            return create_message_handler(              //
                [handler = std::move(handler)]          //
                (const ID&, const void*, std::uint64_t) //
                { handler(); }
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
        else if constexpr (std::is_invocable_v<Handler, const ID&, const void*, std::uint64_t>)
        {
            using callable_t = Callable<Handler, const ID&, const void*, std::uint64_t>;
            return std::make_unique<callable_t>(std::move(handler));
        }
        else if constexpr (std::is_invocable_v<Handler, const AutoCast&>)
        {
            return create_message_handler(                        //
                [handler = std::move(handler)]                    //
                (const ID&, const void* data, std::uint64_t size) //
                { handler(AutoCast{data, &size}); }
            );
        }
        else if constexpr (std::is_invocable_v<Handler, const void*, std::uint64_t>)
        {
            return create_message_handler(                        //
                [handler = std::move(handler)]                    //
                (const ID&, const void* data, std::uint64_t size) //
                { handler(data, size); }
            );
        }
        else
        {
            unreachable<Handler>();
        }
    }
}
