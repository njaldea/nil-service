// TODO: revive test. disabled due to api changes

// #include <nil/service/codec.hpp>
// #include <nil/service/concat.hpp>
// #include <nil/service/consume.hpp>
// #include <nil/service/map.hpp>

// #include <gmock/gmock.h>
// #include <gtest/gtest.h>

// namespace nil::service
// {
//     template <>
//     struct codec<char>
//     {
//         static std::vector<std::uint8_t> serialize(char data)
//         {
//             return {std::uint8_t(data)};
//         }

//         static char deserialize(const void* data, std::uint64_t& size)
//         {
//             size -= 1;
//             return *static_cast<const char*>(data);
//         }
//     };
// }

// class TestService: public nil::service::IStandaloneService
// {
// public:
//     explicit TestService(std::string peer_id)
//         : id(std::move(peer_id))
//     {
//     }

//     TestService() = delete;
//     TestService(TestService&&) = delete;
//     TestService(const TestService&) = delete;
//     TestService& operator=(TestService&&) = delete;
//     TestService& operator=(const TestService&) = delete;
//     ~TestService() noexcept override = default;

//     void run() override
//     {
//         nil::service::detail::invoke(handlers.on_connect, nil::service::ID{id});
//     }

//     void stop() override
//     {
//         nil::service::detail::invoke(handlers.on_disconnect, nil::service::ID{id});
//     }

//     void restart() override
//     {
//     }

//     void publish(std::vector<std::uint8_t> message) override
//     {
//         nil::service::detail::invoke(
//             handlers.on_message,
//             nil::service::ID{id},
//             message.data(),
//             message.size()
//         );
//     }

//     void send(const nil::service::ID& target_id, std::vector<std::uint8_t> message) override
//     {
//         (void)target_id;
//         (void)message;
//     }

//     using IMessagingService::publish;
//     using IMessagingService::send;

//     using IObservableService::on_connect;
//     using IObservableService::on_disconnect;
//     using IObservableService::on_message;
//     using IObservableService::on_ready;

// private:
//     std::string id;
// };

// TEST(BaseService, on_message)
// {
//     const testing::InSequence _;
//     testing::StrictMock<testing::MockFunction<void(std::string)>> mock;

//     TestService service("peer id");
//     service.on_connect([&]() { mock.Call("connect"); });
//     service.on_disconnect([&]() { mock.Call("disconnect"); });
//     service.on_message([&]() { mock.Call("message"); });

//     EXPECT_CALL(mock, Call("connect")).Times(1);
//     service.run();
//     EXPECT_CALL(mock, Call("message")).Times(1);
//     service.publish("abc", 4);
//     EXPECT_CALL(mock, Call("message")).Times(1);
//     service.publish({'a', 'b', 'c'});
//     EXPECT_CALL(mock, Call("message")).Times(1);
//     service.publish(nil::service::concat('a', 'b', 'c'));
//     EXPECT_CALL(mock, Call("disconnect")).Times(1);
//     service.stop();
// }

// TEST(BaseService, on_message_with_id)
// {
//     const testing::InSequence _;
//     testing::StrictMock<testing::MockFunction<void(std::string)>> mock;

//     TestService service("peer id");
//     service.on_connect([&]() { mock.Call("connect"); });
//     service.on_disconnect([&]() { mock.Call("disconnect"); });
//     service.on_message(
//         [&](const nil::service::ID& id)
//         {
//             mock.Call(id.text);
//             mock.Call("message");
//         }
//     );

//     EXPECT_CALL(mock, Call("connect")).Times(1);
//     service.run();
//     EXPECT_CALL(mock, Call("peer id")).Times(1);
//     EXPECT_CALL(mock, Call("message")).Times(1);
//     service.publish("abc", 4);
//     EXPECT_CALL(mock, Call("peer id")).Times(1);
//     EXPECT_CALL(mock, Call("message")).Times(1);
//     service.publish({'a', 'b', 'c'});
//     EXPECT_CALL(mock, Call("peer id")).Times(1);
//     EXPECT_CALL(mock, Call("message")).Times(1);
//     service.publish(nil::service::concat('a', 'b', 'c'));
//     EXPECT_CALL(mock, Call("disconnect")).Times(1);
//     service.stop();
// }

// TEST(BaseService, on_message_with_id_with_raw)
// {
//     const testing::InSequence _;
//     testing::StrictMock<testing::MockFunction<void(std::string)>> mock;

//     TestService service("peer id");
//     service.on_connect([&]() { mock.Call("connect"); });
//     service.on_disconnect([&]() { mock.Call("disconnect"); });
//     service.on_message(                                                       //
//         [&](const nil::service::ID& id, const void* data, std::uint64_t size) //
//         {
//             mock.Call(id.text);
//             mock.Call(nil::service::consume<std::string>(data, size));
//         }
//     );

//     EXPECT_CALL(mock, Call("connect")).Times(1);
//     service.run();
//     EXPECT_CALL(mock, Call("peer id")).Times(1);
//     EXPECT_CALL(mock, Call("abc")).Times(1);
//     service.publish("abc", 3);
//     EXPECT_CALL(mock, Call("peer id")).Times(1);
//     EXPECT_CALL(mock, Call("abc")).Times(1);
//     service.publish({'a', 'b', 'c'});
//     EXPECT_CALL(mock, Call("peer id")).Times(1);
//     EXPECT_CALL(mock, Call("abc")).Times(1);
//     service.publish(nil::service::concat('a', 'b', 'c'));
//     EXPECT_CALL(mock, Call("disconnect")).Times(1);
//     service.stop();
// }

// TEST(BaseService, on_message_with_id_with_codec)
// {
//     const testing::InSequence _;
//     testing::StrictMock<testing::MockFunction<void(std::string)>> mock;

//     TestService service("peer id");
//     service.on_connect([&]() { mock.Call("connect"); });
//     service.on_disconnect([&]() { mock.Call("disconnect"); });
//     service.on_message(                                          //
//         [&](const nil::service::ID& id, const std::string& data) //
//         {
//             mock.Call(id.text);
//             mock.Call(data);
//         }
//     );

//     EXPECT_CALL(mock, Call("connect")).Times(1);
//     service.run();
//     EXPECT_CALL(mock, Call("peer id")).Times(1);
//     EXPECT_CALL(mock, Call("abc")).Times(1);
//     service.publish("abc", 3);
//     EXPECT_CALL(mock, Call("peer id")).Times(1);
//     EXPECT_CALL(mock, Call("abc")).Times(1);
//     service.publish({'a', 'b', 'c'});
//     EXPECT_CALL(mock, Call("peer id")).Times(1);
//     EXPECT_CALL(mock, Call("abc")).Times(1);
//     service.publish(nil::service::concat('a', 'b', 'c'));
//     EXPECT_CALL(mock, Call("disconnect")).Times(1);
//     service.stop();
// }

// TEST(BaseService, on_message_without_id_with_raw)
// {
//     const testing::InSequence _;
//     testing::StrictMock<testing::MockFunction<void(std::string)>> mock;

//     TestService service("peer id");
//     service.on_connect([&]() { mock.Call("connect"); });
//     service.on_disconnect([&]() { mock.Call("disconnect"); });
//     service.on_message(                           //
//         [&](const void* data, std::uint64_t size) //
//         { mock.Call(nil::service::consume<std::string>(data, size)); }
//     );

//     EXPECT_CALL(mock, Call("connect")).Times(1);
//     service.run();
//     EXPECT_CALL(mock, Call("abc")).Times(1);
//     service.publish("abc", 3);
//     EXPECT_CALL(mock, Call("abc")).Times(1);
//     service.publish({'a', 'b', 'c'});
//     EXPECT_CALL(mock, Call("abc")).Times(1);
//     service.publish(nil::service::concat('a', 'b', 'c'));
//     EXPECT_CALL(mock, Call("disconnect")).Times(1);
//     service.stop();
// }

// TEST(BaseService, on_message_without_id_with_codec)
// {
//     const testing::InSequence _;
//     testing::StrictMock<testing::MockFunction<void(std::string)>> mock;

//     TestService service("peer id");
//     service.on_connect([&]() { mock.Call("connect"); });
//     service.on_disconnect([&]() { mock.Call("disconnect"); });
//     service.on_message([&](const std::string& message) { mock.Call(message); });

//     EXPECT_CALL(mock, Call("connect")).Times(1);
//     service.run();
//     EXPECT_CALL(mock, Call("abc")).Times(1);
//     service.publish("abc", 3);
//     EXPECT_CALL(mock, Call("abc")).Times(1);
//     service.publish({'a', 'b', 'c'});
//     EXPECT_CALL(mock, Call("abc")).Times(1);
//     service.publish(nil::service::concat('a', 'b', 'c'));
//     EXPECT_CALL(mock, Call("disconnect")).Times(1);
//     service.stop();
// }

// TEST(BaseService, map_without_id_with_raw)
// {
//     const testing::InSequence _;
//     testing::StrictMock<testing::MockFunction<void(std::string)>> mock;

//     const auto handler //
//         = nil::service::map(nil::service::mapping(
//             'a',
//             [&](const void* d, std::uint64_t s)
//             { mock.Call(nil::service::consume<std::string>(d, s)); }
//         ));
//     {
//         EXPECT_CALL(mock, Call("bc")).Times(1);
//         const std::uint8_t data[] = {'a', 'b', 'c'}; // NOLINT
//         handler({"peer id"}, &data[0], 3);
//     }
//     {
//         const std::uint8_t data[] = {'b', 'b', 'c'}; // NOLINT
//         handler({"peer id"}, &data[0], 3);
//     }
// }

// TEST(BaseService, map_without_id_with_codec)
// {
//     const testing::InSequence _;
//     testing::StrictMock<testing::MockFunction<void(std::string)>> mock;

//     const auto handler                             //
//         = nil::service::map(nil::service::mapping( //
//             'a',
//             [&](const std::string& s) { mock.Call(s); }
//         ));
//     {
//         EXPECT_CALL(mock, Call("bc")).Times(1);
//         const std::uint8_t data[] = {'a', 'b', 'c'}; // NOLINT
//         handler({"peer id"}, &data[0], 3);
//     }
//     {
//         const std::uint8_t data[] = {'b', 'b', 'c'}; // NOLINT
//         handler({"peer id"}, &data[0], 3);
//     }
// }

// TEST(BaseService, on_message_map_with_id_with_raw)
// {
//     const testing::InSequence _;
//     testing::StrictMock<testing::MockFunction<void(std::string)>> mock;

//     const auto handler //
//         = nil::service::map(nil::service::mapping(
//             'a',
//             [&](const nil::service::ID& id, const void* d, std::uint64_t s)
//             {
//                 mock.Call(id.text);
//                 mock.Call(nil::service::consume<std::string>(d, s));
//             }
//         ));
//     {
//         EXPECT_CALL(mock, Call("peer id")).Times(1);
//         EXPECT_CALL(mock, Call("bc")).Times(1);
//         const std::uint8_t data[] = {'a', 'b', 'c'}; // NOLINT
//         handler({"peer id"}, &data[0], 3);
//     }
//     {
//         const std::uint8_t data[] = {'b', 'b', 'c'}; // NOLINT
//         handler({"peer id"}, &data[0], 3);
//     }
// }

// TEST(BaseService, on_message_map_with_id_with_codec)
// {
//     const testing::InSequence _;
//     testing::StrictMock<testing::MockFunction<void(std::string)>> mock;

//     const auto handler //
//         = nil::service::map(nil::service::mapping(
//             'a',
//             [&](const nil::service::ID& id, const std::string& s)
//             {
//                 mock.Call(id.text);
//                 mock.Call(s);
//             }
//         ));
//     {
//         EXPECT_CALL(mock, Call("peer id")).Times(1);
//         EXPECT_CALL(mock, Call("bc")).Times(1);
//         const std::uint8_t data[] = {'a', 'b', 'c'}; // NOLINT
//         handler({"peer id"}, &data[0], 3);
//     }
//     {
//         const std::uint8_t data[] = {'b', 'b', 'c'}; // NOLINT
//         handler({"peer id"}, &data[0], 3);
//     }
// }
