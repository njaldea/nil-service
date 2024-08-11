#include <nil/service/IService.hpp>
#include <nil/service/codec.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

// TODO: add proper checks. currently only checks api calls

class TestService: public nil::service::IService
{
public:
    TestService(std::string peer_id, std::vector<std::uint8_t> data)
        : id(std::move(peer_id))
        , payload(std::move(data))
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
        if (handlers.msg)
        {
            handlers.msg->call({id}, payload.data(), payload.size());
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
        (void)message;
    }

    void send(const nil::service::ID& target_id, std::vector<std::uint8_t> message) override
    {
        (void)target_id;
        (void)message;
    }

private:
    std::string id;
    std::vector<std::uint8_t> payload;
};

TEST(BaseService, on_message)
{
    TestService service("peer id", {0, 1, 2, 3});
    service.on_connect([]() { std::cout << "connect" << std::endl; });
    service.on_disconnect([]() { std::cout << "disconnect" << std::endl; });
    service.on_message([]() { std::cout << "message" << std::endl; });
    service.run();
    service.stop();
}

TEST(BaseService, on_message_with_id)
{
    TestService service("peer id", {0, 1, 2, 3});
    service.on_connect([]() { std::cout << "connect" << std::endl; });
    service.on_disconnect([]() { std::cout << "disconnect" << std::endl; });
    service.on_message([](const nil::service::ID&) { std::cout << "message" << std::endl; });
    service.run();
    service.stop();
}

TEST(BaseService, on_message_with_id_with_raw)
{
    TestService service("peer id", {0, 1, 2, 3});
    service.on_connect([]() { std::cout << "connect" << std::endl; });
    service.on_disconnect([]() { std::cout << "disconnect" << std::endl; });
    service.on_message(                                                      //
        [](const nil::service::ID& id, const void* data, std::uint64_t size) //
        {
            (void)id;
            (void)data;
            (void)size;
            std::cout << "message" << std::endl;
        }
    );
    service.run();
    service.stop();
}

TEST(BaseService, on_message_with_id_with_codec)
{
    TestService service("peer id", {0, 1, 2, 3});
    service.on_connect([]() { std::cout << "connect" << std::endl; });
    service.on_disconnect([]() { std::cout << "disconnect" << std::endl; });
    service.on_message(                                         //
        [](const nil::service::ID& id, const std::string& data) //
        {
            (void)id;
            (void)data;
            std::cout << "message" << std::endl;
        }
    );
    service.run();
    service.stop();
}

TEST(BaseService, on_message_without_id_with_raw)
{
    TestService service("peer id", {0, 1, 2, 3});
    service.on_connect([]() { std::cout << "connect" << std::endl; });
    service.on_disconnect([]() { std::cout << "disconnect" << std::endl; });
    service.on_message(                          //
        [](const void* data, std::uint64_t size) //
        {
            (void)data;
            (void)size;
            std::cout << "message" << std::endl;
        }
    );
    service.run();
    service.stop();
}

TEST(BaseService, on_message_without_id_with_codec)
{
    TestService service("peer id", {0, 1, 2, 3});
    service.on_connect([]() { std::cout << "connect" << std::endl; });
    service.on_disconnect([]() { std::cout << "disconnect" << std::endl; });
    service.on_message(                //
        [](const std::string& message) //
        { std::cout << "message: " << message << std::endl; }
    );
    service.run();
    service.stop();
}
