#include <nil/service/IService.hpp>
#include <nil/service/codec.hpp>
#include <nil/service/concat.hpp>
#include <nil/service/split.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

// TODO: add proper checks. currently only checks api calls

namespace nil::service
{
    template <>
    struct codec<char>
    {
        static std::vector<std::uint8_t> serialize(const char& data)
        {
            return {std::uint8_t(data)};
        }

        static char deserialize(const void* data, std::uint64_t& size)
        {
            size -= 1;
            return *static_cast<const char*>(data);
        }
    };
}

class TestService: public nil::service::IService
{
public:
    explicit TestService(std::string peer_id)
        : id(std::move(peer_id))
    {
    }

    TestService() = delete;
    TestService(TestService&&) = delete;
    TestService(const TestService&) = delete;
    TestService& operator=(TestService&&) = delete;
    TestService& operator=(const TestService&) = delete;
    ~TestService() noexcept override = default;

    void run() override
    {
        if (handlers.connect)
        {
            handlers.connect->call({id});
        }
    }

    void stop() override
    {
        if (handlers.disconnect)
        {
            handlers.disconnect->call({id});
        }
    }

    void restart() override
    {
    }

    void publish(std::vector<std::uint8_t> message) override
    {
        if (handlers.msg)
        {
            handlers.msg->call({id}, message.data(), message.size());
        }
    }

    void send(const nil::service::ID& target_id, std::vector<std::uint8_t> message) override
    {
        (void)target_id;
        (void)message;
    }

    using nil::service::IService::publish;
    using nil::service::IService::send;

private:
    std::string id;
};

TEST(BaseService, on_message)
{
    TestService service("peer id");
    service.on_connect([]() { std::cout << "connect" << std::endl; });
    service.on_disconnect([]() { std::cout << "disconnect" << std::endl; });
    service.on_message([]() { std::cout << "message" << std::endl; });
    service.run();
    service.publish("abc", 4);
    service.publish({'a', 'b', 'c'});
    service.publish(nil::service::concat('a', 'b', 'c'));
    service.stop();
}

TEST(BaseService, on_message_with_id)
{
    TestService service("peer id");
    service.on_connect([]() { std::cout << "connect" << std::endl; });
    service.on_disconnect([]() { std::cout << "disconnect" << std::endl; });
    service.on_message([](const nil::service::ID&) { std::cout << "message" << std::endl; });
    service.run();
    service.publish("abc", 4);
    service.publish({'a', 'b', 'c'});
    service.publish(nil::service::concat('a', 'b', 'c'));
    service.stop();
}

TEST(BaseService, on_message_with_id_with_raw)
{
    TestService service("peer id");
    service.on_connect([]() { std::cout << "connect" << std::endl; });
    service.on_disconnect([]() { std::cout << "disconnect" << std::endl; });
    service.on_message(                                                      //
        [](const nil::service::ID& id, const void* data, std::uint64_t size) //
        {
            (void)id;
            const auto message = nil::service::type_cast<std::string>(data, size);
            std::cout << "message: " << message << std::endl;
        }
    );
    service.run();
    service.publish("abc", 4);
    service.publish({'a', 'b', 'c'});
    service.publish(nil::service::concat('a', 'b', 'c'));
    service.stop();
}

TEST(BaseService, on_message_with_id_with_codec)
{
    TestService service("peer id");
    service.on_connect([]() { std::cout << "connect" << std::endl; });
    service.on_disconnect([]() { std::cout << "disconnect" << std::endl; });
    service.on_message(                                         //
        [](const nil::service::ID& id, const std::string& data) //
        {
            (void)id;
            (void)data;
            std::cout << "message: " << data << std::endl;
        }
    );
    service.run();
    service.publish("abc", 4);
    service.publish({'a', 'b', 'c'});
    service.publish(nil::service::concat('a', 'b', 'c'));
    service.stop();
}

TEST(BaseService, on_message_without_id_with_raw)
{
    TestService service("peer id");
    service.on_connect([]() { std::cout << "connect" << std::endl; });
    service.on_disconnect([]() { std::cout << "disconnect" << std::endl; });
    service.on_message(                          //
        [](const void* data, std::uint64_t size) //
        {
            const auto message = nil::service::type_cast<std::string>(data, size);
            std::cout << "message: " << message << std::endl;
        }
    );
    service.run();
    service.publish("abc", 4);
    service.publish({'a', 'b', 'c'});
    service.publish(nil::service::concat('a', 'b', 'c'));
    service.stop();
}

TEST(BaseService, on_message_without_id_with_codec)
{
    TestService service("peer id");
    service.on_connect([]() { std::cout << "connect" << std::endl; });
    service.on_disconnect([]() { std::cout << "disconnect" << std::endl; });
    service.on_message(                //
        [](const std::string& message) //
        { std::cout << "message: " << message << std::endl; }
    );
    service.run();
    service.publish("abc", 4);
    service.publish({'a', 'b', 'c'});
    service.publish(nil::service::concat('a', 'b', 'c'));
    service.stop();
}

TEST(BaseService, on_message_split_without_id_with_raw)
{
    TestService service("peer id");
    service.on_connect([]() { std::cout << "connect" << std::endl; });
    service.on_disconnect([]() { std::cout << "disconnect" << std::endl; });
    service.on_message(                                //
        nil::service::split<char>(                     //
            [](char f, const void* d, std::uint64_t s) //
            {
                std::cout << "message[0]: " << f << std::endl;
                const auto msg = nil::service::type_cast<std::string>(d, s);
                std::cout << "message[1]: " << msg << std::endl;
            }
        )
    );
    service.run();
    service.publish("abc", 4);
    service.publish({'a', 'b', 'c'});
    service.publish(nil::service::concat('a', 'b', 'c'));
    service.stop();
}

TEST(BaseService, on_message_split_without_id_with_codec)
{
    TestService service("peer id");
    service.on_connect([]() { std::cout << "connect" << std::endl; });
    service.on_disconnect([]() { std::cout << "disconnect" << std::endl; });
    service.on_message(                      //
        nil::service::split<char>(           //
            [](char f, const std::string& s) //
            {
                std::cout << "message[0]: " << f << std::endl;
                std::cout << "message[1]: " << s << std::endl;
            }
        )
    );
    service.run();
    service.publish("abc", 4);
    service.publish({'a', 'b', 'c'});
    service.publish(nil::service::concat('a', 'b', 'c'));
    service.stop();
}

TEST(BaseService, on_message_split_with_id_with_raw)
{
    TestService service("peer id");
    service.on_connect([]() { std::cout << "connect" << std::endl; });
    service.on_disconnect([]() { std::cout << "disconnect" << std::endl; });
    service.on_message(                                                         //
        nil::service::split<char>(                                              //
            [](const nil::service::ID&, char f, const void* d, std::uint64_t s) //
            {
                std::cout << "message[0]: " << f << std::endl;
                const auto msg = nil::service::type_cast<std::string>(d, s);
                std::cout << "message[1]: " << msg << std::endl;
            }
        )
    );
    service.run();
    service.publish("abc", 4);
    service.publish({'a', 'b', 'c'});
    service.publish(nil::service::concat('a', 'b', 'c'));
    service.stop();
}

TEST(BaseService, on_message_split_with_id_with_codec)
{
    TestService service("peer id");
    service.on_connect([]() { std::cout << "connect" << std::endl; });
    service.on_disconnect([]() { std::cout << "disconnect" << std::endl; });
    service.on_message(                                               //
        nil::service::split<char>(                                    //
            [](const nil::service::ID&, char f, const std::string& s) //
            {
                std::cout << "message[0]: " << f << std::endl;
                std::cout << "message[1]: " << s << std::endl;
            }
        )
    );
    service.run();
    service.publish("abc", 4);
    service.publish({'a', 'b', 'c'});
    service.publish(nil::service::concat('a', 'b', 'c'));
    service.stop();
}
