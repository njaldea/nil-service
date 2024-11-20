#include <nil/service.hpp>

#include <nil/clix.hpp>
#include <nil/clix/prebuilt/Help.hpp>

#include <iostream>
#include <thread>

void add_help(nil::clix::Node& node)
{
    flag(node, "help", {.skey = 'h', .msg = "this help"});
}

void add_port(nil::clix::Node& node)
{
    number(node, "port", {.skey = 'p', .msg = "port", .fallback = 0});
}

template <typename R, typename T>
auto create_server(const nil::clix::Options& options, R (*maker)(T))
{
    return maker(T{.host = "127.0.0.1", .port = std::uint16_t(number(options, "port"))});
}

template <typename R, typename T>
auto create_client(const nil::clix::Options& options, R (*maker)(T))
{
    return maker(T{.host = "127.0.0.1", .port = std::uint16_t(number(options, "port"))});
}

template <typename R>
auto create_self(const nil::clix::Options& options, R (*maker)())
{
    (void)options;
    return maker();
}

auto input_output(auto& service)
{
    using namespace std::string_literals;
    std::string message;
    std::uint32_t type = 0;
    while (std::getline(std::cin, message))
    {
        if (message == "reconnect")
        {
            break;
        }

        publish(
            service,
            nil::service::concat(type, "typed > "s, message, " : "s, "secondary here"s)
        );

        type = (type + 1) % 2;
    }
}

template <typename R, typename... T>
struct Runner
{
    Runner(R (*init_creator)(const nil::clix::Options&, R (*)(T...)), R (*init_maker)(T...))
        : creator(init_creator)
        , maker(init_maker)
    {
    }

    int operator()(const nil::clix::Options& options) const
    {
        if (flag(options, "help"))
        {
            help(options, std::cout);
            return 0;
        }
        auto service = creator(options, maker);
        {
            on_message(
                service,
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
            on_ready(                                                       //
                service,                                                    //
                [](const auto& id) {                                        //
                    std::cout << "local        : " << id.text << std::endl; //
                }
            );
            on_connect(
                service,                                                    //
                [](const nil::service::ID& id) {                            //
                    std::cout << "connected    : " << id.text << std::endl; //
                }
            );
            on_disconnect(
                service,                                                    //
                [](const nil::service::ID& id) {                            //
                    std::cout << "disconnected : " << id.text << std::endl; //
                }
            );
        }

        {
            while (true)
            {
                std::thread t1([&]() { start(service); });
                input_output(service);
                stop(service);
                t1.join();
                restart(service);
            }
        }
        return 0;
    }

    R (*creator)(const nil::clix::Options&, R (*)(T...));
    R (*maker)(T...);
};

template <typename SR, typename SO, typename CR, typename CO>
struct CS_Sub
{
    CS_Sub(SR (*init_server_maker)(SO), CR (*init_client_maker)(CO))
        : server_maker(init_server_maker)
        , client_maker(init_client_maker)
    {
    }

    void operator()(nil::clix::Node& node) const
    {
        add_help(node);
        use(node, nil::clix::prebuilt::Help(&std::cout));
        sub(node,
            "server",
            "server",
            [this](auto& nn)
            {
                add_help(nn);
                add_port(nn);
                use(nn, Runner(&create_server, server_maker));
            });
        sub(node,
            "client",
            "client",
            [this](auto& nn)
            {
                add_help(nn);
                add_port(nn);
                use(nn, Runner(&create_client, client_maker));
            });
    }

    SR (*server_maker)(SO);
    CR (*client_maker)(CO);
};

void add_http_node(nil::clix::Node& node)
{
    add_help(node);
    add_port(node);

    constexpr auto http_runner = [](const nil::clix::Options& options)
    {
        if (flag(options, "help"))
        {
            help(options, std::cout);
            return 0;
        }
        auto server = nil::service::http::server::create(
            {.host = "0.0.0.0",
             .port = std::uint16_t(number(options, "port")),
             .buffer = 1024ul * 1024ul * 100ul}
        );
        on_get(
            server,
            [](const nil::service::HTTPTransaction& transaction) -> void
            {
                if ("/" != get_route(transaction))
                {
                    return;
                }
                set_content_type(transaction, "text/html");
                send(
                    transaction,
                    "<!DOCTYPE html>"
                    "<html lang=\"en\">"
                    "<head>"
                    "<script type=\"module\">"
                    "const foo = (host, port) => {"
                    "    let nil = new WebSocket(`ws://${host}:${port}/ws`);"
                    "    nil.binaryType = \"arraybuffer\";"
                    "    nil.onopen = () => console.log(\"open\");"
                    "    nil.onclose = () => console.log(\"close\");"
                    "    nil.onmessage = async (e) => {"
                    "        const data = new Uint8Array(event.data);"
                    "        const tag = new DataView(data.buffer)"
                    "            .getUint32(0, false);"
                    "        const rest = new TextDecoder().decode(data.slice(4));"
                    "        console.log(tag, rest);"
                    "    };"
                    "    return nil;"
                    "};"
                    "globalThis.foo = foo;"
                    "</script>"
                    "</head>"
                    "<body>hello world</body>"
                );
            }
        );
        on_ready(
            server,                                                   //
            [](const auto& id)                                        //
            { std::cout << "ready      : " << id.text << std::endl; } //
        );
        auto ws = use_ws(server, "/ws");
        on_ready(
            ws,                                                       //
            [](const auto& id)                                        //
            { std::cout << "ready      : " << id.text << std::endl; } //
        );
        on_connect(
            ws,                                                       //
            [](const auto& id)                                        //
            { std::cout << "connect    : " << id.text << std::endl; } //
        );
        on_disconnect(
            ws,                                                       //
            [](const auto& id)                                        //
            { std::cout << "disconnect : " << id.text << std::endl; } //
        );
        on_message(
            ws,                                                                             //
            [](const auto& id, const std::string& content)                                  //
            { std::cout << "message    : " << id.text << "  :  " << content << std::endl; } //
        );

        {
            while (true)
            {
                std::thread t1([&]() { start(server); });
                input_output(ws);
                stop(server);
                t1.join();
                restart(server);
            }
        }
        return 0;
    };
    use(node, http_runner);
}

int main(int argc, const char** argv)
{
    auto node = nil::clix::create_node();
    add_help(node);
    use(node, nil::clix::prebuilt::Help(&std::cout));
    sub(node,
        "self",
        "use self protocol",
        [](auto& n)
        {
            add_help(n);
            use(n, Runner(&create_self, &nil::service::self::create));
        });
    sub(node,
        "udp",
        "use udp protocol",
        CS_Sub(&nil::service::udp::server::create, &nil::service::udp::client::create));
    sub(node,
        "tcp",
        "use tcp protocol",
        CS_Sub(&nil::service::tcp::server::create, &nil::service::tcp::client::create));
    sub(node,
        "ws",
        "use ws protocol",
        CS_Sub(&nil::service::ws::server::create, &nil::service::ws::client::create));

    sub(node, "http", "serve http server", add_http_node);
    nil::clix::run(node, argc, argv);
}
