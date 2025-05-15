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

void add_route(nil::clix::Node& node)
{
    param(node, "route", {.skey = 'r', .msg = "route", .fallback = "/"});
}

template <auto maker>
auto create_options(const nil::clix::Options& options)
{
    return maker({.host = "127.0.0.1", .port = std::uint16_t(number(options, "port"))});
}

template <auto maker>
auto create_ws_sc(const nil::clix::Options& options)
{
    return maker(
        {.host = "127.0.0.1",
         .port = std::uint16_t(number(options, "port")),
         .route = param(options, "route")}
    );
}

auto create_wss_server(const nil::clix::Options& options)
{
    return nil::service::wss::server::create(
        {.cert = "../sandbox",
         .host = "127.0.0.1",
         .port = std::uint16_t(number(options, "port")),
         .route = param(options, "route")}
    );
}

auto create_self(const nil::clix::Options& /* options */)
{
    return nil::service::self::create();
}

template <auto maker>
auto create_http_server(const nil::clix::Options& options)
{
    return maker(
        {.host = "0.0.0.0",
         .port = std::uint16_t(number(options, "port")),
         .buffer = 1024ul * 1024ul * 100ul}
    );
}

#ifdef NIL_SERVICE_SSL

template <auto maker>
auto create_https_server(const nil::clix::Options& options)
{
    return maker(
        {.cert = "../sandbox",
         .host = "0.0.0.0",
         .port = std::uint16_t(number(options, "port")),
         .buffer = 1024ul * 1024ul * 100ul}
    );
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

template <auto server_maker, auto client_maker, auto... option_adder>
void sc_node(nil::clix::Node& node)
{
    add_help(node);
    use(node, nil::clix::prebuilt::Help(&std::cout));
    sub(node,
        "server",
        "server",
        [](auto& nn)
        {
            (option_adder(nn), ...);
            use(nn, runner<server_maker>);
        });
    sub(node,
        "client",
        "client",
        [](auto& nn)
        {
            (option_adder(nn), ...);
            use(nn, runner<client_maker>);
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

template <auto create, auto... option_adder>
void add_web_node(nil::clix::Node& node)
{
    (option_adder(node), ...);

    use(node,
        [](const nil::clix::Options& options)
        {
            if (flag(options, "help"))
            {
                help(options, std::cout);
                return 0;
            }
            auto server = create(options);
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
            use(n, runner<&create_self>);
        });
    sub(node,
        "udp",
        "use udp protocol",
        sc_node<
            create_options<&nil::service::udp::server::create>,
            create_options<&nil::service::udp::client::create>,
            add_help,
            add_port>);
    sub(node,
        "tcp",
        "use tcp protocol",
        sc_node<
            create_options<&nil::service::tcp::server::create>,
            create_options<&nil::service::tcp::client::create>,
            add_help,
            add_port>);
    sub(node,
        "ws",
        "use ws protocol",
        sc_node<
            create_ws_sc<&nil::service::ws::server::create>,
            create_ws_sc<&nil::service::ws::client::create>,
            add_help,
            add_port,
            add_route>);
#ifdef NIL_SERVICE_SSL
    sub(node,
        "wss",
        "use wss protocol",
        sc_node< //
            create_wss_server,
            create_ws_sc<&nil::service::wss::client::create>,
            add_help,
            add_port,
            add_route>);
#endif
    sub(node,
        "http",
        "serve http server",
        &add_web_node< //
            create_http_server<&nil::service::http::server::create>,
            add_help,
            add_port>);

#ifdef NIL_SERVICE_SSL
    sub(node,
        "https",
        "serve https server",
        &add_web_node< //
            create_https_server<&nil::service::https::server::create>,
            add_help,
            add_port>);
#endif

    nil::clix::run(node, argc, argv);
}
