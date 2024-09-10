#include <nil/clix.hpp>
#include <nil/clix/node.hpp>
#include <nil/clix/prebuilt/Help.hpp>
#include <nil/service.hpp>

#include <iostream>
#include <thread>

template <typename T>
    requires                                        //
    std::is_same_v<T, nil::service::udp::Server>    //
    || std::is_same_v<T, nil::service::tcp::Server> //
    || std::is_same_v<T, nil::service::ws::Server>
T make_service(const nil::clix::Options& options)
{
    return T({.port = std::uint16_t(number(options, "port"))});
}

template <typename T>
    requires                                        //
    std::is_same_v<T, nil::service::udp::Client>    //
    || std::is_same_v<T, nil::service::tcp::Client> //
    || std::is_same_v<T, nil::service::ws::Client>
T make_service(const nil::clix::Options& options)
{
    return T({.host = "127.0.0.1", .port = std::uint16_t(number(options, "port"))});
}

template <typename T>
    requires std::is_same_v<T, nil::service::self::Server>
T make_service(const nil::clix::Options& options)
{
    (void)options;
    return T();
}

template <typename T>
void add_end_node(nil::clix::Node& node)
{
    flag(node, "help", {.skey = 'h', .msg = "this help"});
    if constexpr (!std::is_same_v<T, nil::service::self::Server>)
    {
        number(
            node,
            "port",
            {
                .skey = 'p',
                .msg = "port",
                .fallback = 8000,
                .implicit = 8000 //
            }
        );
    }
    use(node,
        [](const nil::clix::Options& options)
        {
            if (flag(options, "help"))
            {
                help(options, std::cout);
                return 0;
            }
            auto service = make_service<T>(options);
            {
                service.on_message( //
                    nil::service::map(
                        nil::service::mapping(
                            0u,
                            [](const auto& id, const std::string& m)
                            {
                                std::cout << "from         : " << id.text << std::endl;
                                std::cout << "type         : " << 0 << std::endl;
                                std::cout << "message      : " << m << std::endl;
                            }
                        ),
                        nil::service::mapping(
                            1u,
                            [](const auto& id, const void* data, std::uint64_t size)
                            {
                                const auto m = nil::service::consume<std::string>(data, size);
                                std::cout << "from         : " << id.text << std::endl;
                                std::cout << "type         : " << 1 << std::endl;
                                std::cout << "message      : " << m << std::endl;
                            }
                        )
                    )
                );
                service.on_ready(                                               //
                    [](const auto& id) {                                        //
                        std::cout << "local        : " << id.text << std::endl; //
                    }
                );
                service.on_connect(                                             //
                    [](const nil::service::ID& id) {                            //
                        std::cout << "connected    : " << id.text << std::endl; //
                    }
                );
                service.on_disconnect(                                          //
                    [](const nil::service::ID& id) {                            //
                        std::cout << "disconnected : " << id.text << std::endl; //
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

                        service.publish(nil::service::concat(
                            type,
                            "typed > "s,
                            message,
                            " : "s,
                            "secondary here"s
                        ));

                        type = (type + 1) % 2;
                    }
                    service.stop();
                    t1.join();
                    service.restart();
                }
            }
            return 0;
        });
}

template <typename Server, typename Client>
void add_sub_nodes(nil::clix::Node& node)
{
    use(node, nil::clix::prebuilt::Help(&std::cout));
    sub(node, "server", "server", add_end_node<Server>);
    sub(node, "client", "client", add_end_node<Client>);
}

void add_http_node(nil::clix::Node& node)
{
    flag(node, "help", {.skey = 'h', .msg = "this help"});
    number(
        node,
        "port",
        {
            .skey = 'p',
            .msg = "port",
            .fallback = 8080,
            .implicit = 8080 //
        }
    );
    use(node,
        [](const nil::clix::Options& options)
        {
            if (flag(options, "help"))
            {
                help(options, std::cout);
                return 0;
            }
            nil::service::http::Server server(
                {.port = std::uint16_t(number(options, "port")), .buffer = 1024}
            );
            server.use(
                "/",
                "text/html",
                [](std::ostream& oss)
                {
                    oss << "<!DOCTYPE html>"                                          //
                        << "<html lang=\"en\">"                                       //
                        << "<head>"                                                   //
                        << "<script type=\"module\">"                                 //
                        << "const foo = () => {"                                      //
                        << "    let nil = new WebSocket(\"ws://localhost:8080/ws\");" //
                        << "    nil.onopen = () => console.log(\"open\");"            //
                        << "    nil.onclose = () => console.log(\"close\");"          //
                        << "    nil.onmessage = (e) => console.log(\"message\", e);"  //
                        << "    return nil;"                                          //
                        << "};"                                                       //
                        << "globalThis.nil = foo();"                                  //
                        << "</script>"                                                //
                        << "</head>"                                                  //
                        << "<body>hello world</body>";
                }
            );
            server.on_ready(                                              //
                [](const auto& id)                                        //
                { std::cout << "ready      : " << id.text << std::endl; } //
            );
            auto& ws = server.use_ws("/ws");
            ws.on_ready(                                                  //
                [](const auto& id)                                        //
                { std::cout << "ready      : " << id.text << std::endl; } //
            );
            ws.on_connect(                                                //
                [](const auto& id)                                        //
                { std::cout << "connect    : " << id.text << std::endl; } //
            );
            ws.on_disconnect(                                             //
                [](const auto& id)                                        //
                { std::cout << "disconnect : " << id.text << std::endl; } //
            );
            ws.on_message(                                                                      //
                [](const auto& id, const std::string& content)                                  //
                { std::cout << "message    : " << id.text << "  :  " << content << std::endl; } //
            );
            server.run();
            return 0;
        });
}

int main(int argc, const char** argv)
{
    using namespace nil::service;

    auto root = nil::clix::create_node();
    use(root, nil::clix::prebuilt::Help(&std::cout));
    sub(root, "self", "use self protocol", add_end_node<self::Server>);
    sub(root, "udp", "use udp protocol", add_sub_nodes<udp::Server, udp::Client>);
    sub(root, "tcp", "use tcp protocol", add_sub_nodes<tcp::Server, tcp::Client>);
    sub(root, "ws", "use ws protocol", add_sub_nodes<ws::Server, ws::Client>);
    sub(root, "http", "serve http server", add_http_node);
    return run(root, argc, argv);
}
