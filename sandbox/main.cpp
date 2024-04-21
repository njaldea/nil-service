#include <nil/clix.hpp>
#include <nil/clix/prebuilt/Help.hpp>
#include <nil/service.hpp>

#include <iostream>
#include <thread>

template <typename Service>
typename Service::Options make_service_option(const nil::clix::Options& options)
{
    const auto port = std::uint16_t(options.number("port"));
    constexpr auto is_server //
        = std::is_same_v<nil::service::tcp::Server, Service>
        || std::is_same_v<nil::service::udp::Server, Service>
        || std::is_same_v<nil::service::ws::Server, Service>;
    constexpr auto is_client //
        = std::is_same_v<nil::service::tcp::Client, Service>
        || std::is_same_v<nil::service::udp::Client, Service>
        || std::is_same_v<nil::service::ws::Client, Service>;
    if constexpr (is_server)
    {
        return typename Service::Options{.port = port};
    }
    else if constexpr (is_client)
    {
        return typename Service::Options{.host = "127.0.0.1", .port = port};
    }
    else
    {
        static_assert(
            std::is_same_v<nil::service::tcp::Server, Service>    //
            || std::is_same_v<nil::service::tcp::Client, Service> //
            || std::is_same_v<nil::service::udp::Server, Service> //
            || std::is_same_v<nil::service::udp::Client, Service> //
            || std::is_same_v<nil::service::ws::Server, Service>  //
            || std::is_same_v<nil::service::ws::Client, Service>
        );
    }
}

int help(const nil::clix::Options& options)
{
    options.help(std::cout);
    return 0;
}

template <typename T>
void add_end_node(nil::clix::Node& node)
{
    node.flag("help", {.skey = 'h', .msg = "this help"});
    node.number(
        "port",
        {
            .skey = 'p',
            .msg = "port",
            .fallback = 8000,
            .implicit = 8000 //
        }
    );
    node.runner(
        [](const nil::clix::Options& options)
        {
            if (options.flag("help"))
            {
                return help(options);
            }
            auto service = T(make_service_option<T>(options));
            {
                service.on_message(                             //
                    nil::service::TypedHandler<std::uint32_t>() //
                        .add(
                            0u,
                            [](const std::string& id, const std::string& message)
                            {
                                std::cout << "from         : " << id << std::endl;
                                std::cout << "type         : " << 0 << std::endl;
                                std::cout << "message      : " << message << std::endl;
                            }
                        )
                        .add(
                            1u,
                            [](const std::string& id, const std::string& message)
                            {
                                std::cout << "from         : " << id << std::endl;
                                std::cout << "type         : " << 1 << std::endl;
                                std::cout << "message      : " << message << std::endl;
                            }
                        )
                );
                service.on_connect(             //
                    [](const std::string& id) { //
                        std::cout << "connected    : " << id << std::endl;
                    }
                );
                service.on_disconnect(          //
                    [](const std::string& id) { //
                        std::cout << "disconnected : " << id << std::endl;
                    }
                );
            }

            {
                using namespace std::string_literals;
                while (true)
                {
                    std::thread t1([&]() { service.run(); });
                    std::string message;
                    std::uint32_t type = 0;
                    while (std::getline(std::cin, message))
                    {
                        if (message == "reconnect")
                        {
                            break;
                        }

                        service.publish(type, "typed > " + message, " : "s, "secondary here"s);

                        type = (type + 1) % 2;
                    }
                    service.stop();
                    t1.join();
                    service.restart();
                }
            }
            return 0;
        }
    );
}

template <typename Server, typename Client>
void add_sub_nodes(nil::clix::Node& node)
{
    node.runner(help);
    node.add("server", "server", add_end_node<Server>);
    node.add("client", "client", add_end_node<Client>);
}

int main(int argc, const char** argv)
{
    using nil::clix::Node;
    using namespace nil::service;

    Node root;
    root.runner(help);
    root.add("udp", "use udp protocol", add_sub_nodes<udp::Server, udp::Client>);
    root.add("tcp", "use tcp protocol", add_sub_nodes<tcp::Server, tcp::Client>);
    root.add("ws", "use ws protocol", add_sub_nodes<ws::Server, ws::Client>);
    root.add(
        "self",
        "use self protocol",
        [](nil::clix::Node& node)
        {
            node.flag("help", {.skey = 'h', .msg = "this help"});
            node.runner(
                [](const nil::clix::Options& options)
                {
                    if (options.flag("help"))
                    {
                        return help(options);
                    }
                    nil::service::Self service;
                    {
                        service.on_message(                             //
                            nil::service::TypedHandler<std::uint32_t>() //
                                .add(
                                    0u,
                                    [](const std::string& id, const std::string& message)
                                    {
                                        std::cout << "from         : " << id << std::endl;
                                        std::cout << "type         : " << 0 << std::endl;
                                        std::cout << "message      : " << message << std::endl;
                                    }
                                )
                                .add(
                                    1u,
                                    [](const std::string& id, const std::string& message)
                                    {
                                        std::cout << "from         : " << id << std::endl;
                                        std::cout << "type         : " << 1 << std::endl;
                                        std::cout << "message      : " << message << std::endl;
                                    }
                                )
                        );
                        service.on_connect(             //
                            [](const std::string& id) { //
                                std::cout << "connected    : " << id << std::endl;
                            }
                        );
                        service.on_disconnect(          //
                            [](const std::string& id) { //
                                std::cout << "disconnected : " << id << std::endl;
                            }
                        );
                    }

                    {
                        using namespace std::string_literals;
                        while (true)
                        {
                            std::thread t1([&]() { service.run(); });
                            std::string message;
                            std::uint32_t type = 0;
                            while (std::getline(std::cin, message))
                            {
                                if (message == "reconnect")
                                {
                                    break;
                                }

                                service
                                    .publish(type, "typed > " + message, " : "s, "secondary here"s);

                                type = (type + 1) % 2;
                            }
                            service.stop();
                            t1.join();
                            service.restart();
                        }
                    }
                    return 0;
                }
            );
        }
    );
    return root.run(argc, argv);
}
