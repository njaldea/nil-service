#include <nil/clix.hpp>
#include <nil/clix/prebuilt/Help.hpp>
#include <nil/service.hpp>

#include <iostream>
#include <thread>

int help(const nil::clix::Options& options)
{
    options.help(std::cout);
    return 0;
}

template <typename T>
    requires                                        //
    std::is_same_v<T, nil::service::udp::Server>    //
    || std::is_same_v<T, nil::service::tcp::Server> //
    || std::is_same_v<T, nil::service::ws::Server>
T make_service(const nil::clix::Options& options)
{
    return T({.port = std::uint16_t(options.number("port"))});
}

template <typename T>
    requires                                        //
    std::is_same_v<T, nil::service::udp::Client>    //
    || std::is_same_v<T, nil::service::tcp::Client> //
    || std::is_same_v<T, nil::service::ws::Client>
T make_service(const nil::clix::Options& options)
{
    return T({.host = "127.0.0.1", .port = std::uint16_t(options.number("port"))});
}

template <typename T>
    requires std::is_same_v<T, nil::service::Self>
T make_service(const nil::clix::Options& options)
{
    (void)options;
    return T();
}

template <typename T>
void add_end_node(nil::clix::Node& node)
{
    node.flag("help", {.skey = 'h', .msg = "this help"});
    if constexpr (!std::is_same_v<T, nil::service::Self>)
    {
        node.number(
            "port",
            {
                .skey = 'p',
                .msg = "port",
                .fallback = 8000,
                .implicit = 8000 //
            }
        );
    }
    node.runner(
        [](const nil::clix::Options& options)
        {
            if (options.flag("help"))
            {
                return help(options);
            }
            auto service = make_service<T>(options);
            {
                service.on_message(                             //
                    nil::service::TypedHandler<std::uint32_t>() //
                        .add(
                            0u,
                            [](const nil::service::ID& id, const std::string& message)
                            {
                                std::cout << "from         : " << id.text << std::endl;
                                std::cout << "type         : " << 0 << std::endl;
                                std::cout << "message      : " << message << std::endl;
                            }
                        )
                        .add(
                            1u,
                            [](const nil::service::ID& id, const std::string& message)
                            {
                                std::cout << "from         : " << id.text << std::endl;
                                std::cout << "type         : " << 1 << std::endl;
                                std::cout << "message      : " << message << std::endl;
                            }
                        )
                );
                service.on_connect(                  //
                    [](const nil::service::ID& id) { //
                        std::cout << "connected    : " << id.text << std::endl;
                    }
                );
                service.on_disconnect(               //
                    [](const nil::service::ID& id) { //
                        std::cout << "disconnected : " << id.text << std::endl;
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
    root.add("self", "use self protocol", add_end_node<Self>);
    return root.run(argc, argv);
}
