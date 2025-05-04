#include <nil/service/codec.hpp>
#include <nil/xalt/type_id.hpp>

#include "utils.hpp"

namespace nil::service::detail
{
    std::size_t serialize(xalt::type_id<std::string> tag, void* output, const std::string& message)
    {
        std::copy(message.begin(), message.end(), static_cast<std::uint8_t*>(output));
        return size(tag, message);
    }

    std::string deserialize(
        xalt::type_id<std::string> /* t */,
        const void* input,
        std::uint64_t size
    )
    {
        return std::string(static_cast<const char*>(input), size); // NOLINT
    }

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define NIL_SERVICE_CODEC_DEFINE(TYPE)                                                             \
    std::size_t serialize(xalt::type_id<TYPE> tag, void* output, TYPE data)                        \
    {                                                                                              \
        const auto typed = utils::to_array(data);                                                  \
        std::copy(typed.begin(), typed.end(), static_cast<std::uint8_t*>(output));                 \
        return size(tag, data);                                                                    \
    }                                                                                              \
    TYPE deserialize(xalt::type_id<TYPE>, const void* input, std::uint64_t size)                   \
    {                                                                                              \
        (void)size;                                                                                \
        return utils::from_array<TYPE>(static_cast<const std::uint8_t*>(input));                   \
    }

    NIL_SERVICE_CODEC_DEFINE(std::uint8_t);
    NIL_SERVICE_CODEC_DEFINE(std::uint16_t);
    NIL_SERVICE_CODEC_DEFINE(std::uint32_t);
    NIL_SERVICE_CODEC_DEFINE(std::uint64_t);

    NIL_SERVICE_CODEC_DEFINE(std::int8_t);
    NIL_SERVICE_CODEC_DEFINE(std::int16_t);
    NIL_SERVICE_CODEC_DEFINE(std::int32_t);
    NIL_SERVICE_CODEC_DEFINE(std::int64_t);
#undef NIL_SERVICE_CODEC_DEFINE
}
