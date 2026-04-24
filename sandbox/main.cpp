#include <nil/service.hpp>

#include <nil/clix.hpp>
#include <nil/clix/prebuilt/Help.hpp>

#include <nil/xalt/fn_sign.hpp>
#include <nil/xalt/literal.hpp>
#include <nil/xalt/tlist.hpp>

#include <iostream>
#include <thread>

void add_help(nil::clix::Node& node)
{
    flag(node, "help", {.skey = 'h', .msg = "this help"});
}

void add_server_port(nil::clix::Node& node)
{
    number(node, "port", {.skey = 'p', .msg = "port", .fallback = 0});
}

void add_client_port(nil::clix::Node& node)
{
    number(node, "port", {.skey = 'p', .msg = "port"});
}

void add_route(nil::clix::Node& node)
{
    param(node, "route", {.skey = 'r', .msg = "route", .fallback = "/"});
}

void add_pipe_options(nil::clix::Node& node)
{
    param(node, "read", {.msg = "fifo path to read from", .fallback = "/tmp/pipe-input"});
    param(node, "write", {.msg = "fifo path to write to", .fallback = "/tmp/pipe-output"});
    flag(
        node,
        "flipped",
        {.skey = 'f', .msg = "flip the read/write fifo paths for the peer process"}
    );
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
    return nil::service::ws::server::create(
        {.host = "127.0.0.1",
         .port = std::uint16_t(number(options, "port")),
         .route = param(options, "route")}
    );
}

auto create_self(const nil::clix::Options& /* options */)
{
    return nil::service::self::create();
}

auto create_pipe(const nil::clix::Options& options)
{
    return nil::service::pipe::create({
        [read_path = param(options, "read"),
         write_path = param(options, "write"),
         flipped = flag(options, "flipped")]()
        {
            auto final_read_path = read_path;
            auto final_write_path = write_path;

            if (flipped)
            {
                std::swap(final_read_path, final_write_path);
            }

            return nil::service::pipe::Options{
                .make_read
                = [path = final_read_path]() { return nil::service::pipe::r_mkfifo(path); },
                .make_write
                = [path = final_write_path]() { return nil::service::pipe::mkfifo(path); },
            };
        }() //
    });
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

        service.publish(nil::service::concat(
            type == 0 ? 'A' : 'B',
            "typed > ",
            message,
            " : ",
            "secondary here"
        ));

        type = (type + 1) % 2;
    }
}

template <typename T, typename U>
void loop(T& service, U& io)
{
    while (true)
    {
        std::thread t1([&]() { service.run(); });
        input_output(io);
        service.stop();
        t1.join();
        service.restart();
    }
}

template <typename T>
void handlers(T& service)
{
    service.on_ready(                                                     //
        [](const auto& id) {                                              //
            std::cout << "local        : " << to_string(id) << std::endl; //
        }
    );
    service.on_connect([](const nil::service::ID& id) {               //
        std::cout << "connected    : " << to_string(id) << std::endl; //
    });
    service.on_disconnect([](const nil::service::ID& id) {            //
        std::cout << "disconnected : " << to_string(id) << std::endl; //
    });
    service.on_message(
        [](const auto& id, const auto* /* data */, auto size)
        { std::cout << "message recieved: " << to_string(id) << ":" << size << std::endl; }
    );
    service.on_message(nil::service::map(
        nil::service::mapping(
            'A',
            [](const auto& id, const std::string& m)
            {
                std::cout << "from         : " << to_string(id) << std::endl;
                std::cout << "type         : " << 0 << std::endl;
                std::cout << "message      : " << m << std::endl;
            }
        ),
        nil::service::mapping(
            'B',
            [](const auto& id, const void* data, std::uint64_t size)
            {
                const auto m = nil::service::consume<std::string>(data, size);
                std::cout << "from         : " << to_string(id) << std::endl;
                std::cout << "type         : " << 1 << std::endl;
                std::cout << "message      : " << m << std::endl;
            }
        )
    ));
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
    handlers(*service);
    loop(*service, *service);
    return 0;
}

template <nil::xalt::literal cmd, nil::xalt::literal desc, auto maker, auto... option_adders>
struct sub_cmd
{
    void operator()(nil::clix::Node& node)
    {
        sub(node,
            nil::xalt::literal_v<cmd>,
            nil::xalt::literal_v<desc>,
            [](auto& nn)
            {
                (option_adders(nn), ...);
                use(nn, runner<maker>);
            });
    }
};

template <typename... Ts>
struct sub_cmds
{
    void operator()(nil::clix::Node& node)
    {
        add_help(node);
        use(node, nil::clix::prebuilt::Help(&std::cout));
        (Ts()(node), ...);
    }
};

nil::service::IEventService* add_web_service(nil::service::IWebService& server)
{
    server.on_get(
        [](nil::service::WebTransaction& transaction) -> bool
        {
            if ("/" != get_route(transaction))
            {
                return false;
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
            return true;
        }
    );
    server.on_ready([](const auto& id)                                              //
                    { std::cout << "ready      : " << to_string(id) << std::endl; } //
    );
    auto* ws = server.use_ws("/ws");
    handlers(*ws);
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
            auto ws = add_web_service(*server);
            loop(*server, *ws);
            return 0;
        });
}

int main(int argc, const char** argv)
{
    auto node = nil::clix::make_node();
    add_help(node);
    use(node, nil::clix::prebuilt::Help(&std::cout));
    sub(node,
        "gateway",
        "use gateway",
        [](auto& n)
        {
            add_help(n);
            add_server_port(n);
            add_route(n);
            add_pipe_options(n);
            use(n,
                [](const nil::clix::Options& options)
                {
                    namespace ns = nil::service;
                    auto pipe = create_pipe(options);
                    auto udp = create_options<&ns::udp::server::create>(options);
                    auto tcp = create_options<&ns::tcp::server::create>(options);
                    auto ws = create_ws_sc<&ns::ws::server::create>(options);

                    auto gateway = nil::service::gateway::create();
                    gateway->add_service(*pipe);
                    gateway->add_service(*udp);
                    gateway->add_service(*tcp);
                    gateway->add_service(*ws);

                    handlers(*gateway);

                    std::this_thread::sleep_for(std::chrono::seconds(1));

                    std::vector<std::thread> v;
                    v.emplace_back([&]() { pipe->run(); });
                    v.emplace_back([&]() { udp->run(); });
                    v.emplace_back([&]() { tcp->run(); });
                    v.emplace_back([&]() { ws->run(); });
                    v.emplace_back([&]() { input_output(*gateway); });

                    gateway->run();

                    return 0;
                });
        });
    sub_cmd<"self", "use self protocol", &create_self, &add_help>()(node);
    sub_cmd<"pipe", "use pipe protocol", &create_pipe, &add_help, &add_pipe_options>()(node);
    sub(node,
        "udp",
        "use udp protocol",
        sub_cmds<
            sub_cmd<
                "server",
                "server",
                &create_options<&nil::service::udp::server::create>,
                add_help,
                add_server_port>,
            sub_cmd<
                "client",
                "client",
                &create_options<&nil::service::udp::client::create>,
                add_help,
                add_client_port>>());
    sub(node,
        "tcp",
        "use tcp protocol",
        sub_cmds<
            sub_cmd<
                "server",
                "server",
                &create_options<&nil::service::tcp::server::create>,
                add_help,
                add_server_port>,
            sub_cmd<
                "client",
                "client",
                &create_options<&nil::service::tcp::client::create>,
                add_help,
                add_client_port>>());
    sub(node,
        "ws",
        "use ws protocol",
        sub_cmds<
            sub_cmd<
                "server",
                "server",
                &create_options<&nil::service::ws::server::create>,
                add_help,
                add_server_port,
                add_route>,
            sub_cmd<
                "client",
                "client",
                &create_options<&nil::service::ws::client::create>,
                add_help,
                add_client_port,
                add_route>>());
    sub(node,
        "http",
        "serve http server",
        &add_web_node< //
            create_http_server<&nil::service::http::server::create>,
            add_help,
            add_server_port>);

    nil::clix::run(node, argc, argv);
}
