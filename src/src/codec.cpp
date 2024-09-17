#include <nil/service/codec.hpp>

#include "utils.hpp"

namespace nil::service::detail
{
    std::vector<std::uint8_t> serialize(tag<std::string> t, const std::string& message)
    {
        (void)t;
        return {message.begin(), message.end()};
    }

    std::string deserialize(tag<std::string> t, const void* data, std::uint64_t& size)
    {
        (void)t;
        return std::string(static_cast<const char*>(data), std::exchange(size, 0ul));
    }

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define NIL_SERVICE_CODEC_DEFINE(TYPE)                                                             \
    std::vector<std::uint8_t> serialize(tag<TYPE>, TYPE message)                                   \
    {                                                                                              \
        const auto typed = utils::to_array(message);                                               \
        return std::vector<std::uint8_t>(typed.begin(), typed.end());                              \
    }                                                                                              \
                                                                                                   \
    TYPE deserialize(tag<TYPE>, const void* data, std::uint64_t& size)                             \
    {                                                                                              \
        size -= sizeof(TYPE);                                                                      \
        return utils::from_array<TYPE>(static_cast<const std::uint8_t*>(data));                    \
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
