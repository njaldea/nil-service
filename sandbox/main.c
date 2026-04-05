#include <nil/clix.h>
#include <nil/service.h>

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum run_mode
{
	MODE_GATEWAY,
	MODE_PIPE,
	MODE_UDP_SERVER,
	MODE_UDP_CLIENT,
	MODE_TCP_SERVER,
	MODE_TCP_CLIENT,
	MODE_WS_SERVER,
	MODE_WS_CLIENT,
	MODE_HTTP,
	MODE_SELF
};

typedef struct service_runner
{
	nil_service_runnable runnable;
	nil_service_message io_message;
} service_runner;

typedef struct web_bundle
{
	nil_service_web web;
	nil_service_event ws_event;
} web_bundle;

typedef struct gateway_bundle
{
	nil_service_gateway gateway;
	nil_service_standalone udp;
	nil_service_standalone tcp;
	nil_service_standalone ws;
} gateway_bundle;

typedef struct runner_context
{
	enum run_mode mode;
} runner_context;

typedef struct sub_builder_context
{
	enum run_mode server_mode;
	enum run_mode client_mode;
	int needs_route;
} sub_builder_context;

typedef struct pipe_fd_provider_context
{
	int (*open_fn)(const char* path);
	char* path;
} pipe_fd_provider_context;

static const int64_t g_port_fallback = 0;
static const char* const g_route_fallback = "/";
static const double g_udp_timeout = 2.0;
static const uint64_t g_default_buffer = 1024;
static const uint64_t g_http_buffer = 1024ULL * 1024ULL * 100ULL;

static const runner_context g_mode_gateway = {.mode = MODE_GATEWAY};
static const runner_context g_mode_pipe = {.mode = MODE_PIPE};
static const runner_context g_mode_udp_server = {.mode = MODE_UDP_SERVER};
static const runner_context g_mode_udp_client = {.mode = MODE_UDP_CLIENT};
static const runner_context g_mode_tcp_server = {.mode = MODE_TCP_SERVER};
static const runner_context g_mode_tcp_client = {.mode = MODE_TCP_CLIENT};
static const runner_context g_mode_ws_server = {.mode = MODE_WS_SERVER};
static const runner_context g_mode_ws_client = {.mode = MODE_WS_CLIENT};
static const runner_context g_mode_http = {.mode = MODE_HTTP};
static const runner_context g_mode_self = {.mode = MODE_SELF};

static const sub_builder_context g_udp_sub = {
	.server_mode = MODE_UDP_SERVER,
	.client_mode = MODE_UDP_CLIENT,
	.needs_route = 0,
};

static const sub_builder_context g_tcp_sub = {
	.server_mode = MODE_TCP_SERVER,
	.client_mode = MODE_TCP_CLIENT,
	.needs_route = 0,
};

static const sub_builder_context g_ws_sub = {
	.server_mode = MODE_WS_SERVER,
	.client_mode = MODE_WS_CLIENT,
	.needs_route = 1,
};

static void write_stdout(const char* data, uint64_t size, void* context)
{
	(void)context;
	if (data == NULL || size == 0U)
	{
		return;
	}
	(void)fwrite(data, 1U, (size_t)size, stdout);
}

static int str_eq(const char* lhs, const char* rhs)
{
	if (lhs == NULL || rhs == NULL)
	{
		return 0;
	}
	return strcmp(lhs, rhs) == 0;
}

static char* dup_cstr(const char* value)
{
	char* copy = NULL;
	size_t len = 0;

	if (value == NULL)
	{
		return NULL;
	}

	len = strlen(value);
	copy = (char*)malloc(len + 1U);
	if (copy == NULL)
	{
		return NULL;
	}

	memcpy(copy, value, len + 1U);
	return copy;
}

static int pipe_fd_provider_exec(void* context)
{
	pipe_fd_provider_context* provider = (pipe_fd_provider_context*)context;
	if (provider == NULL || provider->open_fn == NULL || provider->path == NULL)
	{
		return -1;
	}

	return provider->open_fn(provider->path);
}

static void pipe_fd_provider_cleanup(void* context)
{
	pipe_fd_provider_context* provider = (pipe_fd_provider_context*)context;
	if (provider == NULL)
	{
		return;
	}

	free(provider->path);
	free(provider);
}

static void add_help(nil_clix_node node)
{
	nil_clix_node_flag(
		node,
		"help",
		(nil_clix_flag_info){.skey = "h", .msg = "this help"}
	);
}

static void add_server_port(nil_clix_node node)
{
	nil_clix_node_number(node,
		"port",
		(nil_clix_number_info){
			.skey = "p", .msg = "port", .fallback = &g_port_fallback, .implicit = NULL
		});
}

static void add_client_port(nil_clix_node node)
{
	nil_clix_node_number(node,
		"port",
		(nil_clix_number_info){
			.skey = "p", .msg = "port", .fallback = NULL, .implicit = NULL
		});
}

static void add_route(nil_clix_node node)
{
	nil_clix_node_param(node,
		"route",
		(nil_clix_param_info){.skey = "r", .msg = "route", .fallback = g_route_fallback});
}

static void add_pipe_read(nil_clix_node node)
{
	nil_clix_node_param(node,
		"read",
		(nil_clix_param_info){.skey = NULL, .msg = "read fifo path", .fallback = "/tmp/pipe-input"});
}

static void add_pipe_write(nil_clix_node node)
{
	nil_clix_node_param(node,
		"write",
		(nil_clix_param_info){.skey = NULL, .msg = "write fifo path", .fallback =  "/tmp/pipe-output"});
}

static void add_pipe_flipped(nil_clix_node node)
{
	nil_clix_node_flag(
		node,
		"flipped",
		(nil_clix_flag_info){.skey = "f", .msg = "swap read and write fifo handles"}
	);
}

static void on_ready(const nil_service_id* id, void* context)
{
	(void)context;
	printf("local        : ");
	nil_service_id_print(*id);
}

static void on_connect(const nil_service_id* id, void* context)
{
	(void)context;
	printf("connected    : ");
	nil_service_id_print(*id);
}

static void on_disconnect(const nil_service_id* id, void* context)
{
	(void)context;
	printf("disconnected : ");
	nil_service_id_print(*id);
}

static void on_message(const nil_service_id* id, const void* data, uint64_t size, void* context)
{
	const uint8_t* payload = (const uint8_t*)data;
	const char* text = "";
	uint64_t text_size = 0;
	char tag = '?';

	(void)context;

	printf("message recieved: ");
	nil_service_id_print(*id);
	printf("size         : %llu\n", (unsigned long long)size);

	if (payload != NULL && size > 0U)
	{
		tag = (char)payload[0];
		text = (const char*)(payload + 1U);
		text_size = size - 1U;
	}

	printf("from         : ");
	nil_service_id_print(*id);
	printf("type         : %d\n", tag == 'A' ? 0 : 1);
	printf("message      : ");
	if (text_size > 0U)
	{
		(void)fwrite(text, 1U, (size_t)text_size, stdout);
	}
	printf("\n");
}

static int route_equals(nil_service_web_transaction transaction, const char* target)
{
	uint64_t route_size = 0;
	const char* route = nil_service_web_transaction_get_route(transaction, &route_size);
	const size_t target_size = strlen(target);

	if (route == NULL)
	{
		return 0;
	}

	if (route_size != (uint64_t)target_size)
	{
		return 0;
	}

	return memcmp(route, target, target_size) == 0;
}

static int on_http_get(nil_service_web_transaction transaction, void* context)
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
		  "        const rest = new TextDecoder().decode(data.slice(4));"
		  "        console.log(tag, rest);"
		  "    };"
		  "    return nil;"
		  "};"
		  "globalThis.foo = foo;"
		  "</script>"
		  "</head>"
		  "<body>hello world</body>";

	(void)context;

	if (!route_equals(transaction, "/"))
	{
		return 0;
	}

	nil_service_web_transaction_set_content_type(transaction, "text/html");
	nil_service_web_transaction_send(transaction, body, (uint64_t)(sizeof(body) - 1U));
	return 1;
}

static void on_http_ready(const nil_service_id* id, void* context)
{
	(void)context;
	printf("ready        : ");
	nil_service_id_print(*id);
}

static void attach_handlers(nil_service_callback callback)
{
	nil_service_callback_on_ready(callback,
		(nil_service_callback_info){.exec = on_ready, .context = NULL, .cleanup = NULL});
	nil_service_callback_on_connect(callback,
		(nil_service_callback_info){.exec = on_connect, .context = NULL, .cleanup = NULL});
	nil_service_callback_on_disconnect(callback,
		(nil_service_callback_info){.exec = on_disconnect, .context = NULL, .cleanup = NULL});
	nil_service_callback_on_message(callback,
		(nil_service_msg_callback_info){.exec = on_message, .context = NULL, .cleanup = NULL});
}

static void input_output(nil_service_message service)
{
	char* line = NULL;
	size_t line_cap = 0;
	ssize_t line_size = 0;
	uint32_t type = 0;
	static const char prefix[] = "typed > ";
	static const char middle[] = " : ";
	static const char suffix[] = "secondary here";

	for (;;)
	{
		uint8_t* message = NULL;
		size_t payload_size = 0;
		size_t content_size = 0;

		line_size = getline(&line, &line_cap, stdin);
		if (line_size < 0)
		{
			break;
		}

		if (line_size > 0 && line[(size_t)line_size - 1U] == '\n')
		{
			line[--line_size] = '\0';
		}

		if (str_eq(line, "reconnect"))
		{
			break;
		}

		content_size = (sizeof(prefix) - 1U) + (size_t)line_size + (sizeof(middle) - 1U)
			+ (sizeof(suffix) - 1U);
		payload_size = 1U + content_size;
		message = (uint8_t*)malloc(payload_size);
		if (message == NULL)
		{
			continue;
		}

		message[0] = type == 0U ? (uint8_t)'A' : (uint8_t)'B';
		memcpy(message + 1U, prefix, sizeof(prefix) - 1U);
		memcpy(message + 1U + (sizeof(prefix) - 1U), line, (size_t)line_size);
		memcpy(message + 1U + (sizeof(prefix) - 1U) + (size_t)line_size, middle, sizeof(middle) - 1U);
		memcpy(
			message + 1U + (sizeof(prefix) - 1U) + (size_t)line_size + (sizeof(middle) - 1U),
			suffix,
			sizeof(suffix) - 1U
		);

		nil_service_message_publish(service, message, (uint64_t)payload_size);
		free(message);

		type = (type + 1U) % 2U;
	}

	free(line);
}

static void* start_runner_thread(void* context)
{
	const nil_service_runnable runnable = *(const nil_service_runnable*)context;
	nil_service_runnable_start(runnable);
	return NULL;
}

static void loop_runner(const service_runner* runner)
{
	for (;;)
	{
		pthread_t thread = (pthread_t)0;
		if (pthread_create(&thread, NULL, start_runner_thread, (void*)&runner->runnable) != 0)
		{
			return;
		}
		input_output(runner->io_message);
		nil_service_runnable_stop(runner->runnable);
		(void)pthread_join(thread, NULL);
		nil_service_runnable_restart(runner->runnable);
	}
}

static nil_service_standalone create_standalone(
	enum run_mode mode,
	const char* host,
	uint16_t port,
	const char* route
)
{
	switch (mode)
	{
		case MODE_UDP_SERVER:
			return nil_service_create_udp_server(host, port, g_default_buffer, g_udp_timeout);
		case MODE_UDP_CLIENT:
			return nil_service_create_udp_client(host, port, g_default_buffer, g_udp_timeout);
		case MODE_TCP_SERVER:
			return nil_service_create_tcp_server(host, port, g_default_buffer);
		case MODE_TCP_CLIENT:
			return nil_service_create_tcp_client(host, port, g_default_buffer);
		case MODE_WS_SERVER:
			return nil_service_create_ws_server(host, port, route, g_default_buffer);
		case MODE_WS_CLIENT:
			return nil_service_create_ws_client(host, port, route, g_default_buffer);
		default:
			return (nil_service_standalone){.handle = NULL};
	}
}

static web_bundle create_http_with_ws(uint16_t port)
{
	web_bundle bundle;
	bundle.web = nil_service_create_http_server("0.0.0.0", port, g_http_buffer);
	nil_service_web_on_get(bundle.web,
		(nil_service_web_get_callback_info){
			.exec = on_http_get,
			.context = NULL,
			.cleanup = NULL,
		});
	nil_service_web_on_ready(bundle.web,
		(nil_service_callback_info){.exec = on_http_ready, .context = NULL, .cleanup = NULL});
	bundle.ws_event = nil_service_web_use_ws(bundle.web, "/ws");
	return bundle;
}

static gateway_bundle create_gateway_services(const char* route)
{
	gateway_bundle bundle;
	const uint16_t udp_port = 0U;
	const uint16_t tcp_port = 0U;
	const uint16_t ws_port = 0U;

	bundle.udp = nil_service_create_udp_server("127.0.0.1", udp_port, g_default_buffer, g_udp_timeout);
	bundle.tcp = nil_service_create_tcp_server("127.0.0.1", tcp_port, g_default_buffer);
	bundle.ws = nil_service_create_ws_server("127.0.0.1", ws_port, route, g_default_buffer);
	bundle.gateway = nil_service_create_gateway();

	printf(
		"gateway ports : udp=%u tcp=%u ws=%u\n",
		(unsigned)udp_port,
		(unsigned)tcp_port,
		(unsigned)ws_port
	);

	nil_service_gateway_add_service(bundle.gateway, nil_service_standalone_to_event(bundle.udp));
	nil_service_gateway_add_service(bundle.gateway, nil_service_standalone_to_event(bundle.tcp));
	nil_service_gateway_add_service(bundle.gateway, nil_service_standalone_to_event(bundle.ws));

	return bundle;
}

static int run_standalone(enum run_mode mode, nil_clix_options options)
{
	const uint16_t port = (uint16_t)nil_clix_options_number(options, "port");
	const char* route = NULL;
	nil_service_standalone standalone;
	service_runner runner;

	if (mode == MODE_WS_SERVER || mode == MODE_WS_CLIENT)
	{
		route = nil_clix_options_param(options, "route");
	}

	standalone = create_standalone(mode, "127.0.0.1", port, route);
	if (standalone.handle == NULL)
	{
		return 1;
	}

	attach_handlers(nil_service_standalone_to_callback(standalone));

	runner.runnable = nil_service_standalone_to_runnable(standalone);
	runner.io_message = nil_service_standalone_to_message(standalone);
	loop_runner(&runner);

	nil_service_standalone_destroy(standalone);
	return 0;
}

static int run_pipe(nil_clix_options options)
{
#if defined(__unix__) || defined(__unix) || defined(unix) || defined(__APPLE__)
	const char* read_path = nil_clix_options_param(options, "read");
	const char* write_path = nil_clix_options_param(options, "write");
	nil_service_standalone standalone;
	service_runner runner;

	if (nil_clix_options_flag(options, "flipped") != 0)
	{
		const char* tmp = read_path;
		read_path = write_path;
		write_path = tmp;
	}

	{
		pipe_fd_provider_context* read_provider
			= (pipe_fd_provider_context*)malloc(sizeof(pipe_fd_provider_context));
		pipe_fd_provider_context* write_provider
			= (pipe_fd_provider_context*)malloc(sizeof(pipe_fd_provider_context));

		if (read_provider == NULL || write_provider == NULL)
		{
			free(read_provider);
			free(write_provider);
			return 1;
		}

		read_provider->open_fn = nil_service_pipe_r_mkfifo;
		read_provider->path = dup_cstr(read_path);
		write_provider->open_fn = nil_service_pipe_mkfifo;
		write_provider->path = dup_cstr(write_path);

		if (read_provider->path == NULL || write_provider->path == NULL)
		{
			pipe_fd_provider_cleanup(read_provider);
			pipe_fd_provider_cleanup(write_provider);
			return 1;
		}

		standalone = nil_service_create_pipe(
			(nil_service_pipe_fd_provider){
				.exec = pipe_fd_provider_exec,
				.context = read_provider,
				.cleanup = pipe_fd_provider_cleanup,
			},
			(nil_service_pipe_fd_provider){
				.exec = pipe_fd_provider_exec,
				.context = write_provider,
				.cleanup = pipe_fd_provider_cleanup,
			},
			g_default_buffer
		);
	}
	if (standalone.handle == NULL)
	{
		return 1;
	}

	attach_handlers(nil_service_standalone_to_callback(standalone));

	runner.runnable = nil_service_standalone_to_runnable(standalone);
	runner.io_message = nil_service_standalone_to_message(standalone);
	loop_runner(&runner);

	nil_service_standalone_destroy(standalone);
	return 0;
#else
	(void)options;
	fprintf(stderr, "pipe creator is only available on POSIX platforms\n");
	return 1;
#endif
}

static int run_http(nil_clix_options options)
{
	const uint16_t port = (uint16_t)nil_clix_options_number(options, "port");
	web_bundle bundle = create_http_with_ws(port);
	service_runner runner;

	attach_handlers(nil_service_event_to_callback(bundle.ws_event));

	runner.runnable = nil_service_web_to_runnable(bundle.web);
	runner.io_message = nil_service_event_to_message(bundle.ws_event);
	loop_runner(&runner);

	nil_service_web_destroy(bundle.web);
	return 0;
}

static void* start_standalone_thread(void* context)
{
	const nil_service_runnable runnable = *(const nil_service_runnable*)context;
	nil_service_runnable_start(runnable);
	return NULL;
}

static void* run_input_thread(void* context)
{
	nil_service_message service = *(nil_service_message*)context;
	input_output(service);
	return NULL;
}

static int run_gateway(nil_clix_options options)
{
	const char* route = nil_clix_options_param(options, "route");
	gateway_bundle bundle = create_gateway_services(route);

	pthread_t udp_thread = (pthread_t)0;
	pthread_t tcp_thread = (pthread_t)0;
	pthread_t ws_thread = (pthread_t)0;
	pthread_t io_thread = (pthread_t)0;

	nil_service_runnable udp_runnable = nil_service_standalone_to_runnable(bundle.udp);
	nil_service_runnable tcp_runnable = nil_service_standalone_to_runnable(bundle.tcp);
	nil_service_runnable ws_runnable = nil_service_standalone_to_runnable(bundle.ws);
	nil_service_runnable gateway_runnable = nil_service_gateway_to_runnable(bundle.gateway);
	nil_service_message gateway_message = nil_service_gateway_to_message(bundle.gateway);

	attach_handlers(nil_service_gateway_to_callback(bundle.gateway));

	(void)pthread_create(&udp_thread, NULL, start_standalone_thread, (void*)&udp_runnable);
	(void)pthread_create(&tcp_thread, NULL, start_standalone_thread, (void*)&tcp_runnable);
	(void)pthread_create(&ws_thread, NULL, start_standalone_thread, (void*)&ws_runnable);
	(void)pthread_create(&io_thread, NULL, run_input_thread, (void*)&gateway_message);

	nil_service_runnable_start(gateway_runnable);

	(void)pthread_join(io_thread, NULL);
	nil_service_runnable_stop(gateway_runnable);
	nil_service_runnable_stop(udp_runnable);
	nil_service_runnable_stop(tcp_runnable);
	nil_service_runnable_stop(ws_runnable);

	(void)pthread_join(udp_thread, NULL);
	(void)pthread_join(tcp_thread, NULL);
	(void)pthread_join(ws_thread, NULL);

	nil_service_standalone_destroy(bundle.udp);
	nil_service_standalone_destroy(bundle.tcp);
	nil_service_standalone_destroy(bundle.ws);
	nil_service_gateway_destroy(bundle.gateway);

	return 0;
}

static int run_mode_exec(nil_clix_options options, void* context)
{
	const runner_context* rc = (const runner_context*)context;

	if (nil_clix_options_flag(options, "help") != 0)
	{
		nil_clix_options_help(options, (nil_clix_write_info){.exec = write_stdout, .context = NULL});
		return 0;
	}

	switch (rc->mode)
	{
		case MODE_GATEWAY:
			return run_gateway(options);
		case MODE_PIPE:
			return run_pipe(options);
		case MODE_UDP_SERVER:
		case MODE_UDP_CLIENT:
		case MODE_TCP_SERVER:
		case MODE_TCP_CLIENT:
		case MODE_WS_SERVER:
		case MODE_WS_CLIENT:
			return run_standalone(rc->mode, options);
		case MODE_HTTP:
			return run_http(options);
		case MODE_SELF:
			fprintf(stderr, "self creator is not exposed in C API\n");
			return 1;
		default:
			return 1;
	}
}

static int show_help_exec(nil_clix_options options, void* context)
{
	(void)context;
	nil_clix_options_help(options, (nil_clix_write_info){.exec = write_stdout, .context = NULL});
	return 0;
}

static void attach_runner(nil_clix_node node, const runner_context* context)
{
	nil_clix_node_use(node,
		(nil_clix_exec_info){.exec = run_mode_exec, .context = (void*)context, .cleanup = NULL});
}

static void build_server(nil_clix_node node, void* context)
{
	const sub_builder_context* cfg = (const sub_builder_context*)context;
	add_help(node);
	add_server_port(node);
	if (cfg->needs_route)
	{
		add_route(node);
	}
	switch (cfg->server_mode)
	{
		case MODE_UDP_SERVER:
			attach_runner(node, &g_mode_udp_server);
			break;
		case MODE_TCP_SERVER:
			attach_runner(node, &g_mode_tcp_server);
			break;
		case MODE_WS_SERVER:
			attach_runner(node, &g_mode_ws_server);
			break;
		default:
			break;
	}
}

static void build_client(nil_clix_node node, void* context)
{
	const sub_builder_context* cfg = (const sub_builder_context*)context;
	add_help(node);
	add_client_port(node);
	if (cfg->needs_route)
	{
		add_route(node);
	}
	switch (cfg->client_mode)
	{
		case MODE_UDP_CLIENT:
			attach_runner(node, &g_mode_udp_client);
			break;
		case MODE_TCP_CLIENT:
			attach_runner(node, &g_mode_tcp_client);
			break;
		case MODE_WS_CLIENT:
			attach_runner(node, &g_mode_ws_client);
			break;
		default:
			break;
	}
}

static void build_sc(nil_clix_node node, void* context)
{
	const sub_builder_context* cfg = (const sub_builder_context*)context;
	add_help(node);
	nil_clix_node_use(node,
		(nil_clix_exec_info){.exec = show_help_exec, .context = NULL, .cleanup = NULL});
	nil_clix_node_sub(node,
		"server",
		"server",
		(nil_clix_sub_info){.exec = build_server, .context = (void*)cfg, .cleanup = NULL});
	nil_clix_node_sub(node,
		"client",
		"client",
		(nil_clix_sub_info){.exec = build_client, .context = (void*)cfg, .cleanup = NULL});
}

static void build_gateway(nil_clix_node node, void* context)
{
	(void)context;
	add_help(node);
	add_route(node);
	attach_runner(node, &g_mode_gateway);
}

static void build_pipe(nil_clix_node node, void* context)
{
	(void)context;
	add_help(node);
	add_pipe_read(node);
	add_pipe_write(node);
	add_pipe_flipped(node);
	attach_runner(node, &g_mode_pipe);
}

static void build_self(nil_clix_node node, void* context)
{
	(void)context;
	add_help(node);
	attach_runner(node, &g_mode_self);
}

static void build_http(nil_clix_node node, void* context)
{
	(void)context;
	add_help(node);
	add_server_port(node);
	attach_runner(node, &g_mode_http);
}

int main(int argc, const char* const* argv)
{
	nil_clix_node node = nil_clix_node_create();

	add_help(node);
	nil_clix_node_use(node,
		(nil_clix_exec_info){.exec = show_help_exec, .context = NULL, .cleanup = NULL});

	nil_clix_node_sub(node,
		"gateway",
		"use gateway",
		(nil_clix_sub_info){.exec = build_gateway, .context = NULL, .cleanup = NULL});

	nil_clix_node_sub(node,
		"pipe",
		"use posix pipe protocol",
		(nil_clix_sub_info){.exec = build_pipe, .context = NULL, .cleanup = NULL});

	nil_clix_node_sub(node,
		"self",
		"use self protocol",
		(nil_clix_sub_info){.exec = build_self, .context = NULL, .cleanup = NULL});

	nil_clix_node_sub(node,
		"udp",
		"use udp protocol",
		(nil_clix_sub_info){.exec = build_sc, .context = (void*)&g_udp_sub, .cleanup = NULL});

	nil_clix_node_sub(node,
		"tcp",
		"use tcp protocol",
		(nil_clix_sub_info){.exec = build_sc, .context = (void*)&g_tcp_sub, .cleanup = NULL});

	nil_clix_node_sub(node,
		"ws",
		"use ws protocol",
		(nil_clix_sub_info){.exec = build_sc, .context = (void*)&g_ws_sub, .cleanup = NULL});

	nil_clix_node_sub(node,
		"http",
		"serve http server",
		(nil_clix_sub_info){.exec = build_http, .context = NULL, .cleanup = NULL});

	(void)nil_clix_node_run(node, argc, argv);
	nil_clix_node_destroy(node);
	return 0;
}
