#pragma once

#include <nil/xalt/errors.hpp>

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace nil::service
{
    namespace detail
    {
        template <typename T>
        using tag = std::type_identity<T>;

        std::vector<std::uint8_t> serialize(tag<std::string>, const std::string& message);
        std::string deserialize(tag<std::string>, const void* data, std::uint64_t& size);

        // NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define NIL_SERVICE_CODEC_DECLARE(TYPE)                                                            \
    std::vector<std::uint8_t> serialize(tag<TYPE>, TYPE message);                                  \
    TYPE deserialize(tag<TYPE>, const void* data, std::uint64_t& size)

        NIL_SERVICE_CODEC_DECLARE(std::uint8_t);
        NIL_SERVICE_CODEC_DECLARE(std::uint16_t);
        NIL_SERVICE_CODEC_DECLARE(std::uint32_t);
        NIL_SERVICE_CODEC_DECLARE(std::uint64_t);

        NIL_SERVICE_CODEC_DECLARE(std::int8_t);
        NIL_SERVICE_CODEC_DECLARE(std::int16_t);
        NIL_SERVICE_CODEC_DECLARE(std::int32_t);
        NIL_SERVICE_CODEC_DECLARE(std::int64_t);
#undef NIL_SERVICE_CODEC_DECLARE

        template <typename T>
        concept with_tagged_serialize = requires(T arg) {
            {
                nil::service::detail::serialize(std::declval<tag<T>>(), arg)
            } -> std::same_as<std::vector<std::uint8_t>>;
        };
        template <typename T>
        concept with_tagged_deserialize = requires(T arg) {
            {
                nil::service::detail::deserialize(
                    std::declval<tag<T>>(),
                    std::declval<const void*>(),
                    std::declval<std::uint64_t&>()
                )
            } -> std::same_as<T>;
        };
    }

    template <typename T, typename = void>
    struct codec
    {
        /**
         * @brief serializes the data to a buffer
         *
         * @param message
         * @return std::vector<std::uint8_t>
         */
        static std::vector<std::uint8_t> serialize(const T& message)
        {
            if constexpr (detail::with_tagged_serialize<T>)
            {
                return nil::service::detail::serialize(detail::tag<T>(), message);
            }
            else
            {
                xalt::undefined<T>(); // codec serialize method is not implemented
                return {};
            }
        }

        /**
         * @brief deserializes the buffer into a distinct type
         *
         * @param data - buffer to use
         * @param size - available buffer size
         *             - this is expected to be adjusted by the data
         *               consumed during deserialization
         * @return T
         */
        static T deserialize(const void* data, std::uint64_t& size)
        {
            if constexpr (detail::with_tagged_deserialize<T>)
            {
                return nil::service::detail::deserialize(detail::tag<T>(), data, size);
            }
            else
            {
                xalt::undefined<T>(); // codec deserialize method is not implemented
                return {};
            }
        }
    };
}
