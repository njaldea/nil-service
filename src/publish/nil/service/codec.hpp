#pragma once

#include <nil/xalt/errors.hpp>
#include <nil/xalt/type_id.hpp>

#include <cstdint>
#include <string>
#include <utility>

namespace nil::service
{
    namespace detail
    {
        inline std::size_t size(xalt::type_id<std::string> /* tag */, const std::string& message)
        {
            return message.size();
        }

        std::size_t serialize(
            xalt::type_id<std::string> /* tag */,
            void* output,
            const std::string& message
        );

        std::string deserialize(
            xalt::type_id<std::string> /* tag */,
            const void* input,
            std::uint64_t size
        );

        // NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define NIL_SERVICE_CODEC_DECLARE(TYPE)                                                            \
    inline std::size_t size(xalt::type_id<TYPE>, TYPE data)                                        \
    {                                                                                              \
        return sizeof(data);                                                                       \
    }                                                                                              \
    std::size_t serialize(xalt::type_id<TYPE>, void* output, TYPE data);                           \
    TYPE deserialize(xalt::type_id<TYPE>, const void* input, std::uint64_t size)

        NIL_SERVICE_CODEC_DECLARE(std::uint8_t);
        NIL_SERVICE_CODEC_DECLARE(std::uint16_t);
        NIL_SERVICE_CODEC_DECLARE(std::uint32_t);
        NIL_SERVICE_CODEC_DECLARE(std::uint64_t);

        NIL_SERVICE_CODEC_DECLARE(std::int8_t);
        NIL_SERVICE_CODEC_DECLARE(std::int16_t);
        NIL_SERVICE_CODEC_DECLARE(std::int32_t);
        NIL_SERVICE_CODEC_DECLARE(std::int64_t);

        NIL_SERVICE_CODEC_DECLARE(char);
#undef NIL_SERVICE_CODEC_DECLARE

        template <typename T>
        concept with_tagged_size = requires(T arg) {
            { detail::size(std::declval<xalt::type_id<T>>(), arg) } -> std::same_as<std::size_t>;
        };
        template <typename T>
        concept with_tagged_serialize = requires(T arg) {
            {
                detail::serialize(
                    std::declval<xalt::type_id<T>>(),
                    std::declval<void*>(),
                    std::declval<T>()
                )
            } -> std::same_as<std::size_t>;
        };
        template <typename T>
        concept with_tagged_deserialize = requires(T arg) {
            {
                nil::service::detail::deserialize(
                    std::declval<xalt::type_id<T>>(),
                    std::declval<const void*>(),
                    std::declval<std::uint64_t>()
                )
            } -> std::same_as<T>;
        };
    }

    template <typename T, typename = void>
    struct codec
    {
        static std::size_t size(const T& message)
        {
            if constexpr (detail::with_tagged_size<T>)
            {
                return detail::size(xalt::type_id<T>(), message);
            }
            else
            {
                xalt::undefined<T>(); // codec size method is not implemented
            }
        }

        /**
         * @brief serializes the data to a buffer
         *
         * @param output buffer with enough space dictated by codec<T>::size
         * @param data
         * @return size payload consumed. should be the same as codec<T>::size
         */
        static std::size_t serialize(void* output, const T& data)
        {
            if constexpr (detail::with_tagged_serialize<T>)
            {
                return detail::serialize(xalt::type_id<T>(), output, data);
            }
            else
            {
                xalt::undefined<T>(); // codec serialize method is not implemented
            }
        }

        /**
         * @brief deserializes the buffer into a distinct type
         *
         * @param input - buffer to use
         * @param size - available buffer size
         * @return T
         */
        static T deserialize(const void* input, std::uint64_t size)
        {
            if constexpr (detail::with_tagged_deserialize<T>)
            {
                return detail::deserialize(xalt::type_id<T>(), input, size);
            }
            else
            {
                xalt::undefined<T>(); // codec deserialize method is not implemented
            }
        }
    };

    template <typename T, std::size_t N>
    struct codec<T[N]> // NOLINT
    {
        static std::size_t size(const T (&arg)[N]) // NOLINT
        {
            auto size = 0ul;
            for (auto i = 0u; i < N; ++i)
            {
                size += codec<T>::size(arg[i]);
            }
            return size;
        }

        static std::size_t serialize(void* output, const T (&arg)[N]) // NOLINT
        {
            auto* p = static_cast<std::uint8_t*>(output);
            auto size = 0ul;
            for (auto i = 0u; i < N; ++i)
            {
                size += codec<T>::serialize(p + size, arg[i]);
            }
            return size;
        }
    };
}
