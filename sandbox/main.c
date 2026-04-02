#include <nil/service.h>

#include <pthread.h>
#include <stdio.h>
#include <string.h>

enum
{
    GATEWAY_TCP_PORT = 1101,
    GATEWAY_UDP_PORT = 1102,
    WS_HTTP_PORT = 1104,
    GATEWAY_WS_PORT = WS_HTTP_PORT,
    HTTP_PORT = WS_HTTP_PORT,
    HTTP_BUFFER_SIZE = 1024UL * 1024UL * 100UL,
    DEFAULT_BUFFER_SIZE = 2048,
    INPUT_BUFFER_SIZE = 4096,
    MAX_SERVICE_COUNT = 4
};

static const double DEFAULT_UDP_TIMEOUT_S = 1.0;
static const char DEFAULT_ROUTE[] = "/ws";

typedef enum Transport
{
    TRANSPORT_TCP,
    TRANSPORT_UDP,
    TRANSPORT_WS,
    TRANSPORT_HTTP,
    TRANSPORT_GATEWAY
} Transport;

typedef struct AppContext
{
    nil_service_standalone standalone[MAX_SERVICE_COUNT];
    nil_service_runnable runnable[MAX_SERVICE_COUNT];
    size_t service_count;
    nil_service_message message;
    nil_service_callback callback;
    nil_service_gateway gateway;
    nil_service_web web;
} AppContext;

static void input_loop(nil_service_message message_service);

static void print_id(const char* prefix, const nil_service_id* id)
{
    printf("%s", prefix);
    nil_service_id_print(*id);
}

static void on_ready(const nil_service_id* id, void* context)
{
    (void)context;
    print_id("ready        : ", id);
}

static void on_connect(const nil_service_id* id, void* context)
{
    (void)context;
    print_id("connected    : ", id);
}

static void on_disconnect(const nil_service_id* id, void* context)
{
    (void)context;
    print_id("disconnected : ", id);
}

static void on_message(const nil_service_id* id, const void* data, uint64_t size, void* context)
{
    (void)context;
    print_id("message from : ", id);
    printf("message size : %llu\n", (unsigned long long)size);
    printf("message body : %.*s\n", (int)size, (const char*)data);
}

static void* run_service(void* context)
{
    nil_service_runnable* runnable = (nil_service_runnable*)context;
    nil_service_runnable_start(*runnable);
    return NULL;
}

static int is_client_mode(const char* mode)
{
    return mode != NULL && strcmp(mode, "client") == 0;
}

static int is_server_mode(const char* mode)
{
    return mode != NULL && strcmp(mode, "server") == 0;
}

static int is_tcp_transport(const char* transport)
{
    return transport != NULL && strcmp(transport, "tcp") == 0;
}

static int is_udp_transport(const char* transport)
{
    return transport != NULL && strcmp(transport, "udp") == 0;
}

static int is_ws_transport(const char* transport)
{
    return transport != NULL && strcmp(transport, "ws") == 0;
}

static int is_gateway_transport(const char* transport)
{
    return transport != NULL && strcmp(transport, "gateway") == 0;
}

static int is_http_transport(const char* transport)
{
    return transport != NULL && strcmp(transport, "http") == 0;
}

static int is_leaf_transport(const char* transport)
{
    return is_tcp_transport(transport) || is_udp_transport(transport) || is_ws_transport(transport);
}

static int has_http_command(int argc, char** argv)
{
    return argc == 2 && is_http_transport(argv[1]);
}

static int has_gateway_command(int argc, char** argv)
{
    return argc == 2 && is_gateway_transport(argv[1]);
}

static int has_leaf_command(int argc, char** argv)
{
    return argc == 3 && is_leaf_transport(argv[1])
        && (is_client_mode(argv[2]) || is_server_mode(argv[2]));
}

static int has_valid_command(int argc, char** argv)
{
    return has_gateway_command(argc, argv) || has_leaf_command(argc, argv)
        || has_http_command(argc, argv);
}

static Transport parse_transport(int argc, char** argv)
{
    if (has_gateway_command(argc, argv))
    {
        return TRANSPORT_GATEWAY;
    }

    if (has_http_command(argc, argv))
    {
        return TRANSPORT_HTTP;
    }

    if (is_udp_transport(argv[1]))
    {
        return TRANSPORT_UDP;
    }

    if (is_ws_transport(argv[1]))
    {
        return TRANSPORT_WS;
    }

    return TRANSPORT_TCP;
}

static const char* parse_mode(int argc, char** argv)
{
    return argc == 3 ? argv[2] : NULL;
}

static void print_usage(const char* program)
{
    fprintf(stderr, "usage: %s gateway\n", program);
    fprintf(stderr, "   or: %s http\n", program);
    fprintf(stderr, "   or: %s <tcp|udp|ws> <server|client>\n", program);
}

static uint16_t default_port_for_transport(Transport transport)
{
    if (transport == TRANSPORT_UDP)
    {
        return GATEWAY_UDP_PORT;
    }

    if (transport == TRANSPORT_WS)
    {
        return GATEWAY_WS_PORT;
    }

    if (transport == TRANSPORT_HTTP)
    {
        return HTTP_PORT;
    }

    return GATEWAY_TCP_PORT;
}

static nil_service_standalone create_leaf_service(Transport transport, const char* mode, uint16_t port)
{
    const int is_client = is_client_mode(mode);

    if (transport == TRANSPORT_UDP)
    {
        if (is_client)
        {
            return nil_service_create_udp_client(
                "127.0.0.1",
                port,
                DEFAULT_BUFFER_SIZE,
                DEFAULT_UDP_TIMEOUT_S
            );
        }

        return nil_service_create_udp_server(
            "127.0.0.1",
            port,
            DEFAULT_BUFFER_SIZE,
            DEFAULT_UDP_TIMEOUT_S
        );
    }

    if (transport == TRANSPORT_WS)
    {
        if (is_client)
        {
            return nil_service_create_ws_client(
                "127.0.0.1",
                port,
                DEFAULT_ROUTE,
                DEFAULT_BUFFER_SIZE
            );
        }

        return nil_service_create_ws_server(
            "127.0.0.1",
            port,
            DEFAULT_ROUTE,
            DEFAULT_BUFFER_SIZE
        );
    }

    if (is_client)
    {
        return nil_service_create_tcp_client("127.0.0.1", port, DEFAULT_BUFFER_SIZE);
    }

    return nil_service_create_tcp_server("127.0.0.1", port, DEFAULT_BUFFER_SIZE);
}

static void register_callbacks(nil_service_callback callback_service)
{
    nil_service_callback_on_ready(
        callback_service,
        (nil_service_callback_info){.exec = on_ready, .context = NULL, .cleanup = NULL}
    );
    nil_service_callback_on_connect(
        callback_service,
        (nil_service_callback_info){.exec = on_connect, .context = NULL, .cleanup = NULL}
    );
    nil_service_callback_on_disconnect(
        callback_service,
        (nil_service_callback_info){.exec = on_disconnect, .context = NULL, .cleanup = NULL}
    );
    nil_service_callback_on_message(
        callback_service,
        (nil_service_msg_callback_info){.exec = on_message, .context = NULL, .cleanup = NULL}
    );
}

static void register_web_ready(const nil_service_id* id, void* context)
{
    (void)context;
    print_id("web ready    : ", id);
}

static int on_web_get(nil_service_web_transaction transaction, void* context)
{
    uint64_t route_size = 0U;
    const char* route = nil_service_web_transaction_get_route(transaction, &route_size);
    (void)context;

    if (route == NULL)
    {
        return 0;
    }

    if (route_size == 1U && route[0] == '/')
    {
        static const char body[]
            = "<!DOCTYPE html>"
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
              "        const rest = new TextDecoder().decode(data);"
              "        console.log(tag, rest);"
              "    };"
              "    return nil;"
              "};"
              "globalThis.foo = foo;"
              "</script>"
              "</head>"
              "<body>hello world</body>";
        nil_service_web_transaction_set_content_type(transaction, "text/html");
        nil_service_web_transaction_send(transaction, body, sizeof(body) - 1U);
        return 1;
    }

    return 0;
}

static void init_service(AppContext* app, size_t index, nil_service_standalone standalone)
{
    nil_service_event event_service = nil_service_standalone_to_event(standalone);

    app->standalone[index] = standalone;
    app->runnable[index] = nil_service_standalone_to_runnable(standalone);

    if (index == 0U)
    {
        app->message = nil_service_event_to_message(event_service);
        app->callback = nil_service_event_to_callback(event_service);
    }
}

static void build_single_service(AppContext* app, Transport transport, const char* mode)
{
    app->service_count = 1U;
    init_service(app, 0U, create_leaf_service(transport, mode, default_port_for_transport(transport)));
}

static void build_gateway_service(AppContext* app, const char* mode)
{
    app->service_count = 4U;
    app->gateway = nil_service_create_gateway();
    
    init_service(app, 0U, nil_service_gateway_to_standalone(app->gateway));

    init_service(app, 1U, create_leaf_service(TRANSPORT_TCP, mode, GATEWAY_TCP_PORT));
    nil_service_gateway_add_service(app->gateway, nil_service_standalone_to_event(app->standalone[1U]));

    init_service(app, 2U, create_leaf_service(TRANSPORT_UDP, mode, GATEWAY_UDP_PORT));
    nil_service_gateway_add_service(app->gateway, nil_service_standalone_to_event(app->standalone[2U]));

    init_service(app, 3U, create_leaf_service(TRANSPORT_WS, mode, GATEWAY_WS_PORT));
    nil_service_gateway_add_service(app->gateway, nil_service_standalone_to_event(app->standalone[3U]));
}

static void build_app(AppContext* app, Transport transport, const char* mode)
{
    memset(app, 0, sizeof(*app));

    if (transport == TRANSPORT_HTTP)
    {
        nil_service_event ws_event;

        app->service_count = 1U;
        app->web = nil_service_create_http_server(
            "0.0.0.0",
            default_port_for_transport(transport),
            HTTP_BUFFER_SIZE
        );
        ws_event = nil_service_web_use_ws(app->web, DEFAULT_ROUTE);
        app->message = nil_service_event_to_message(ws_event);
        app->callback = nil_service_event_to_callback(ws_event);
        app->runnable[0U] = nil_service_web_to_runnable(app->web);
        return;
    }

    if (transport == TRANSPORT_GATEWAY)
    {
        build_gateway_service(app, mode);
        return;
    }

    build_single_service(app, transport, mode);
}

static void stop_all_services(AppContext* app)
{
    for (size_t index = 0U; index < app->service_count; ++index)
    {
        nil_service_runnable_stop(app->runnable[index]);
    }
}

static void* run_input_loop(void* context)
{
    AppContext* app = (AppContext*)context;
    input_loop(app->message);
    stop_all_services(app);
    return NULL;
}

static int start_subservices(AppContext* app, pthread_t* threads, size_t* started_threads)
{
    *started_threads = 0U;

    for (size_t index = 1U; index < app->service_count; ++index)
    {
        int create_result = pthread_create(
            &threads[*started_threads],
            NULL,
            run_service,
            &app->runnable[index]
        );
        if (create_result != 0)
        {
            fprintf(stderr, "failed to start subservice thread %zu: %d\n", index, create_result);
            return create_result;
        }

        ++(*started_threads);
    }

    return 0;
}

static void join_subservices(pthread_t* threads, size_t started_threads)
{
    for (size_t index = 0U; index < started_threads; ++index)
    {
        pthread_join(threads[index], NULL);
    }
}

static void destroy_services(AppContext* app)
{
    size_t index = app->service_count;

    if (app->web.handle != NULL)
    {
        nil_service_web_destroy(app->web);
        app->web.handle = NULL;
        return;
    }

    if (app->gateway.handle != NULL)
    {
        while (index > 1U)
        {
            --index;
            nil_service_standalone_destroy(app->standalone[index]);
        }

        nil_service_gateway_destroy(app->gateway);
        app->gateway.handle = NULL;
        return;
    }

    while (index > 0U)
    {
        --index;
        nil_service_standalone_destroy(app->standalone[index]);
    }
}

static void input_loop(nil_service_message message_service)
{
    char buffer[INPUT_BUFFER_SIZE];

    while (fgets(buffer, sizeof(buffer), stdin) != NULL)
    {
        size_t length = strlen(buffer);

        if (length > 0U && buffer[length - 1U] == '\n')
        {
            buffer[--length] = '\0';
        }

        if (strcmp(buffer, "quit") == 0)
        {
            break;
        }

        if (length == 0U)
        {
            continue;
        }

        if (message_service.handle == NULL)
        {
            continue;
        }

        nil_service_message_publish(message_service, buffer, (uint64_t)length);
    }
}

int main(int argc, char** argv)
{
    AppContext app;
    pthread_t subservice_threads[MAX_SERVICE_COUNT] = {0};
    pthread_t input_thread = {0};
    size_t started_subservice_threads = 0U;
    int create_result = 0;
    Transport transport = TRANSPORT_TCP;
    const char* mode = NULL;

    if (!has_valid_command(argc, argv))
    {
        print_usage(argv[0]);
        return 1;
    }

    transport = parse_transport(argc, argv);
    mode = parse_mode(argc, argv);

    build_app(&app, transport, mode);

    if (app.web.handle != NULL)
    {
        nil_service_web_on_ready(
            app.web,
            (nil_service_callback_info){.exec = register_web_ready, .context = NULL, .cleanup = NULL}
        );
        nil_service_web_on_get(
            app.web,
            (nil_service_web_get_callback_info){.exec = on_web_get, .context = NULL, .cleanup = NULL}
        );
        register_callbacks(app.callback);
    }
    else
    {
        register_callbacks(app.callback);
    }

    create_result = start_subservices(&app, subservice_threads, &started_subservice_threads);
    if (create_result != 0)
    {
        stop_all_services(&app);
        join_subservices(subservice_threads, started_subservice_threads);
        destroy_services(&app);
        return 1;
    }

    create_result = pthread_create(&input_thread, NULL, run_input_loop, &app);
    if (create_result != 0)
    {
        fprintf(stderr, "failed to start input thread: %d\n", create_result);
        stop_all_services(&app);
        join_subservices(subservice_threads, started_subservice_threads);
        destroy_services(&app);
        return 1;
    }

    nil_service_runnable_start(app.runnable[0]);

    stop_all_services(&app);
    pthread_join(input_thread, NULL);
    join_subservices(subservice_threads, started_subservice_threads);

    destroy_services(&app);
    return 0;
}
