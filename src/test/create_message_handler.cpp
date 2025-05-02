#include <nil/service/detail/create_message_handler.hpp>

#include <gtest/gtest.h>

#include <functional>

TEST(create_message_handler, no_arg)
{
    using nil::service::detail::create_message_handler;
    create_message_handler([]() {});
}

TEST(create_message_handler, one_arg)
{
    using nil::service::detail::create_message_handler;

// NOLINTNEXTLINE
#define THIS_CHECK(id)                                                                             \
    static_assert(std::is_same_v<std::remove_cvref_t<decltype(id)>, nil::service::ID>)

    std::function<void(const nil::service::ID&, const void*, std::uint64_t)> cb;

    // clang-format off
    cb = create_message_handler([](const nil::service::ID& id) { THIS_CHECK(id); });
    cb = create_message_handler([](const auto&             id) { THIS_CHECK(id); });
    cb = create_message_handler([](const std::string&      id) {
        static_assert(std::is_same_v<std::remove_cvref_t<decltype(id)>, std::string>); // actually data
    });
    // clang-format on
#undef THIS_CHECK
}

TEST(create_message_handler, two_args)
{
    using nil::service::detail::create_message_handler;

// NOLINTNEXTLINE
#define THIS_CHECK(id, data)                                                                       \
    static_assert(std::is_same_v<std::remove_cvref_t<decltype(id)>, nil::service::ID>);            \
    static_assert(std::is_same_v<std::remove_cvref_t<decltype(data)>, std::string>)

    std::function<void(const nil::service::ID&, const void*, std::uint64_t)> cb;

    // clang-format off
    cb = create_message_handler([](const nil::service::ID& id, const std::string& data) { THIS_CHECK(id, data); });
    cb = create_message_handler([](const auto&             id, const std::string& data) { THIS_CHECK(id, data); });
    cb = create_message_handler([](const nil::service::ID& id, const auto&        data) {
        static_assert(std::is_same_v<std::remove_cvref_t<decltype(id)>, nil::service::ID>);
         // it is autocast
        static_assert(std::is_same_v<std::remove_cvref_t<decltype(data)>, nil::service::detail::AutoCast>);
    });
    cb = create_message_handler([](const auto&             id, const auto&        data) {
        static_assert(std::is_same_v<std::remove_cvref_t<decltype(id)>, const void*>);      // actually data
        static_assert(std::is_same_v<std::remove_cvref_t<decltype(data)>, std::uint64_t>);  // actually size
    });
    // clang-format on
#undef THIS_CHECK
}

TEST(create_message_handler, three_args)
{
    using nil::service::detail::create_message_handler;
// NOLINTNEXTLINE
#define THIS_CHECK(id, data, size)                                                                 \
    static_assert(std::is_same_v<std::remove_cvref_t<decltype(id)>, nil::service::ID>);            \
    static_assert(std::is_same_v<std::remove_cvref_t<decltype(data)>, const void*>);               \
    static_assert(std::is_same_v<std::remove_cvref_t<decltype(size)>, std::uint64_t>)

    std::function<void(const nil::service::ID&, const void*, std::uint64_t)> cb;

    // clang-format off
    cb = create_message_handler([](const nil::service::ID& id, const void* data, std::uint64_t size) { THIS_CHECK(id, data, size); });
    cb = create_message_handler([](const nil::service::ID& id, const void* data, auto          size) { THIS_CHECK(id, data, size); });
    cb = create_message_handler([](const nil::service::ID& id, auto        data, std::uint64_t size) { THIS_CHECK(id, data, size); });
    cb = create_message_handler([](const nil::service::ID& id, auto        data, auto          size) { THIS_CHECK(id, data, size); });
    cb = create_message_handler([](const auto&             id, const void* data, std::uint64_t size) { THIS_CHECK(id, data, size); });
    cb = create_message_handler([](const auto&             id, const void* data, auto          size) { THIS_CHECK(id, data, size); });
    cb = create_message_handler([](const auto&             id, auto        data, std::uint64_t size) { THIS_CHECK(id, data, size); });
    cb = create_message_handler([](const auto&             id, auto        data, auto          size) { THIS_CHECK(id, data, size); });
    // clang-format on
#undef THIS_CHECK
}
