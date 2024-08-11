#pragma once

#include "IService.hpp"
#include "codec.hpp"

#include <type_traits>
#include <unordered_map>

namespace nil::service
{
    template <typename Indexer>
    class TypedHandler final
    {
    public:
        TypedHandler() = default;
        ~TypedHandler() noexcept = default;

        TypedHandler(TypedHandler&&) = default;
        TypedHandler& operator=(TypedHandler&&) = default;

        TypedHandler(const TypedHandler&) = delete;
        TypedHandler& operator=(const TypedHandler&) = delete;

        template <typename Handler>
        TypedHandler add(Indexer type, Handler handler) &&
        {
            handlers.emplace(type, detail::create_message_handler(handler));
            return std::move(*this);
        }

        template <typename Handler>
        TypedHandler& add(Indexer type, Handler handler) &
        {
            handlers.emplace(type, detail::create_message_handler(std::move(handler)));
            return *this;
        }

        void operator()(const ID& id, const void* data, std::uint64_t size) const
        {
            const auto* const m = static_cast<const std::uint8_t*>(data);
            const auto o_size = size;
            const auto t = codec<Indexer>::deserialize(data, size);
            const auto it = handlers.find(t);
            if (it != handlers.end() && it->second)
            {
                it->second->call(id, m + o_size - size, size);
            }
        }

    private:
        using handler_t = detail::ICallable<const ID&, const void*, std::uint64_t>;
        std::unordered_map<Indexer, std::unique_ptr<handler_t>> handlers;
    };
}
