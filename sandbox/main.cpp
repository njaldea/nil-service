#include "nil/service/http/server/create.hpp"
#include <nil/service.hpp>

#include <nil/clix.hpp>
#include <nil/clix/prebuilt/Help.hpp>

#include <nil/xalt/fn_sign.hpp>
#include <nil/xalt/tlist.hpp>

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

template <typename T>
struct arg_one;

template <typename T>
struct arg_one<nil::xalt::tlist_types<T>>
{
    using type = T;
};

template <auto maker>
auto create_server(const nil::clix::Options& options)
{
    using T = typename arg_one<typename nil::xalt::fn_sign<decltype(maker)>::arg_types>::type;
    return maker(T{.host = "127.0.0.1", .port = std::uint16_t(number(options, "port"))});
}

template <auto maker>
auto create_client(const nil::clix::Options& options)
{
    using T = typename arg_one<typename nil::xalt::fn_sign<decltype(maker)>::arg_types>::type;
    return maker(T{.host = "127.0.0.1", .port = std::uint16_t(number(options, "port"))});
}

template <auto maker>
auto create_self(const nil::clix::Options& options)
{
    (void)options;
    return maker();
}

nil::service::http::server::Options make_http_option(const nil::clix::Options& options)
{
    return {
        .host = "0.0.0.0",
        .port = std::uint16_t(number(options, "port")),
        .buffer = 1024ul * 1024ul * 100ul
    };
}

#ifdef NIL_SERVICE_SSL

nil::service::https::server::Options make_https_option(const nil::clix::Options& options)
{
    return {
        .cert = "",
        .host = "0.0.0.0",
        .port = std::uint16_t(number(options, "port")),
        .buffer = 1024ul * 1024ul * 100ul
    };
}

#endif

auto input_output(auto& service)
{
    std::string message;
    std::uint32_t type = 0;
    while (std::getline(std::cin, message))
    {
        if (message == "reconnect")
        {
            break;
        }

        publish(service, nil::service::concat(type, "typed > ", message, " : ", "secondary here"));

        type = (type + 1) % 2;
    }
}

template <typename T, typename U>
void loop(T& service, U& io)
{
    while (true)
    {
        std::thread t1([&]() { start(service); });
        input_output(io);
        stop(service);
        t1.join();
        restart(service);
    }
}

template <typename T>
void handlers(T& service)
{
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

template <auto creator>
int runner(const nil::clix::Options& options)
{
    if (flag(options, "help"))
    {
        help(options, std::cout);
        return 0;
    }
    auto service = creator(options);
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
    handlers(service);
    loop(service, service);
    return 0;
}

template <auto server_maker, auto client_maker>
void sv_node(nil::clix::Node& node)
{
    add_help(node);
    use(node, nil::clix::prebuilt::Help(&std::cout));
    sub(node,
        "server",
        "server",
        [](auto& nn)
        {
            add_help(nn);
            add_port(nn);
            use(nn, runner<&create_server<server_maker>>);
        });
    sub(node,
        "client",
        "client",
        [](auto& nn)
        {
            add_help(nn);
            add_port(nn);
            use(nn, runner<&create_client<client_maker>>);
        });
}

nil::service::P add_web_service(nil::service::WebService& server)
{
    on_get(
        server,
        [](const nil::service::WebTransaction& transaction) -> void
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
                "    let nil = new WebSocket(`/ws`);"
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
    on_message(
        ws,                                                                             //
        [](const auto& id, const std::string& content)                                  //
        { std::cout << "message    : " << id.text << "  :  " << content << std::endl; } //
    );
    handlers(ws);
    return ws;
}

template <auto create, auto opt>
void add_web_node(nil::clix::Node& node)
{
    add_help(node);
    add_port(node);

    use(node,
        [](const nil::clix::Options& options)
        {
            if (flag(options, "help"))
            {
                help(options, std::cout);
                return 0;
            }
            auto server = create(opt(options));
            auto ws = add_web_service(server);
            loop(server, ws);
            return 0;
        });
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
            use(n, runner<&create_self<&nil::service::self::create>>);
        });
    sub(node,
        "udp",
        "use udp protocol",
        sv_node<&nil::service::udp::server::create, &nil::service::udp::client::create>);
    sub(node,
        "tcp",
        "use tcp protocol",
        sv_node<&nil::service::tcp::server::create, &nil::service::tcp::client::create>);
    sub(node,
        "ws",
        "use ws protocol",
        sv_node<&nil::service::ws::server::create, &nil::service::ws::client::create>);

    sub(node,
        "http",
        "serve http server",
        &add_web_node<&nil::service::http::server::create, &make_http_option>);

#ifdef NIL_SERVICE_SSL
    sub(node,
        "https",
        "serve https server",
        &add_web_node<&nil::service::https::server::create, &make_https_option>);
#endif

    nil::clix::run(node, argc, argv);
}
