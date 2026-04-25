#pragma once

// NOLINTNEXTLINE(hicpp-deprecated-headers,modernize-deprecated-headers)
#include <stddef.h>

// NOLINTNEXTLINE(hicpp-deprecated-headers,modernize-deprecated-headers)
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif
    // Unless explicitly documented otherwise, API functions require valid non-null handles.

    // NOLINTNEXTLINE(modernize-use-using)
    typedef struct nil_service_runnable
    {
        void* handle;
    } nil_service_runnable;

    // NOLINTNEXTLINE(modernize-use-using)
    typedef struct nil_service_message
    {
        void* handle;
    } nil_service_message; // IMessageService

    // NOLINTNEXTLINE(modernize-use-using)
    typedef struct nil_service_callback
    {
        void* handle;
    } nil_service_callback; // ICallbackService

    // NOLINTNEXTLINE(modernize-use-using)
    typedef struct nil_service_event
    {
        void* handle;
    } nil_service_event; // IEventService: IMessageService, ICallbackService

    // NOLINTNEXTLINE(modernize-use-using)
    typedef struct nil_service_standalone
    {
        void* handle;
    } nil_service_standalone; // IStandalone: IEventService, IRunnableService

    // NOLINTNEXTLINE(modernize-use-using)
    typedef struct nil_service_gateway
    {
        void* handle;
    } nil_service_gateway; // IGatewayService

    // NOLINTNEXTLINE(modernize-use-using)
    typedef struct nil_service_web
    {
        void* handle;
    } nil_service_web; // IWebService

    // NOLINTNEXTLINE(modernize-use-using)
    typedef struct nil_service_web_transaction
    {
        void* handle;
    } nil_service_web_transaction; // WebTransaction (valid only inside web on_get callback)

    void nil_service_gateway_add_service(nil_service_gateway gateway, nil_service_event service);

    // NOLINTNEXTLINE(modernize-use-using)
    typedef struct nil_service_dispatch_info
    {
        void (*exec)(void*);
        void* context;
        void (*cleanup)(void*);
    } nil_service_dispatch_info;

    void nil_service_runnable_run(nil_service_runnable service);
    void nil_service_runnable_poll(nil_service_runnable service);
    void nil_service_runnable_stop(nil_service_runnable service);
    void nil_service_runnable_restart(nil_service_runnable service);
    void nil_service_runnable_dispatch(
        nil_service_runnable service,
        nil_service_dispatch_info callback
    );

    // NOLINTNEXTLINE(modernize-use-using)
    typedef struct nil_service_id
    {
        const void* owner;
        const void* id;
        void* _; // std::string (*to_string)(const void*); callback-scoped; not thread-safe.
    } nil_service_id;

    // NOTE:
    // - String conversion is valid only while handling a callback that provided this id.
    // - Not thread-safe.
    // Writes the string representation of `id` into `buffer` (at most `size` bytes,
    // null-terminated). Returns the number of bytes written (excluding null terminator).
    uint64_t nil_service_id_to_string(nil_service_id id, char* buffer, uint64_t size);

    // NOLINTNEXTLINE(modernize-use-using)
    typedef struct nil_service_ids
    {
        uint32_t size;
        nil_service_id* ids;
    } nil_service_ids;

    void nil_service_message_publish(nil_service_message service, const void* data, uint64_t size);
    void nil_service_message_publish_ex(
        nil_service_id id,
        nil_service_message service,
        const void* data,
        uint64_t size
    );
    void nil_service_message_send(
        nil_service_id id,
        nil_service_message service,
        const void* data,
        uint64_t size
    );

    void nil_service_message_publish_ex_ids(
        nil_service_ids ids,
        nil_service_message service,
        const void* data,
        uint64_t size
    );
    void nil_service_message_send_ids(
        nil_service_ids ids,
        nil_service_message service,
        const void* data,
        uint64_t size
    );

    // NOLINTNEXTLINE(modernize-use-using)
    typedef struct nil_service_msg_callback_info
    {
        void (*exec)(const nil_service_id*, const void*, uint64_t, void*);
        void* context;
        void (*cleanup)(void*);
    } nil_service_msg_callback_info;

    // NOLINTNEXTLINE(modernize-use-using)
    typedef struct nil_service_callback_info
    {
        void (*exec)(const nil_service_id* id, void*);
        void* context;
        void (*cleanup)(void*);
    } nil_service_callback_info;

    // NOLINTNEXTLINE(modernize-use-using)
    typedef struct nil_service_web_get_callback_info
    {
        int (*exec)(nil_service_web_transaction* transaction, void*);
        void* context;
        void (*cleanup)(void*);
    } nil_service_web_get_callback_info;

    void nil_service_callback_on_ready(
        nil_service_callback service,
        nil_service_callback_info callback
    );

    void nil_service_callback_on_connect(
        nil_service_callback service,
        nil_service_callback_info callback
    );

    void nil_service_callback_on_disconnect(
        nil_service_callback service,
        nil_service_callback_info callback
    );

    void nil_service_callback_on_message(
        nil_service_callback service,
        nil_service_msg_callback_info callback
    );

    void nil_service_web_on_ready(nil_service_web service, nil_service_callback_info callback);

    void nil_service_web_on_get(
        nil_service_web service,
        nil_service_web_get_callback_info callback
    );

    // Returns a non-owning event-service view backed by `service`.
    // The returned handle must not outlive `service`.
    nil_service_event nil_service_web_use_ws(nil_service_web service, const char* route);

    // `transaction` is only valid during the on_get callback invocation.
    void nil_service_web_transaction_set_content_type(
        nil_service_web_transaction transaction,
        const char* content_type
    );

    // `transaction` is only valid during the on_get callback invocation.
    // `body` is copied into the response before this function returns.
    void nil_service_web_transaction_send(
        nil_service_web_transaction transaction,
        const void* body,
        uint64_t size
    );

    // `transaction` is only valid during the on_get callback invocation.
    // Returned pointer aliases internal request memory and is not null-terminated.
    // If `size` is not null, it receives the route length.
    const char* nil_service_web_transaction_get_route(
        nil_service_web_transaction transaction,
        uint64_t* size
    );

    // clang-format off
    nil_service_message nil_service_event_to_message(nil_service_event service);
    nil_service_callback nil_service_event_to_callback(nil_service_event service);
    
    // Conversion helpers return non-owning views and never transfer ownership.
    // Destroy only the original owning handle (`gateway`, `web`, or `standalone`).
    nil_service_standalone nil_service_gateway_to_standalone(nil_service_gateway service);
    nil_service_event nil_service_gateway_to_event(nil_service_gateway service);
    nil_service_message nil_service_gateway_to_message(nil_service_gateway service);
    nil_service_callback nil_service_gateway_to_callback(nil_service_gateway service);
    nil_service_runnable nil_service_gateway_to_runnable(nil_service_gateway service);

    nil_service_message nil_service_standalone_to_message(nil_service_standalone service);
    nil_service_callback nil_service_standalone_to_callback(nil_service_standalone service);
    nil_service_event nil_service_standalone_to_event(nil_service_standalone service);
    nil_service_runnable nil_service_standalone_to_runnable(nil_service_standalone service);

    nil_service_runnable nil_service_web_to_runnable(nil_service_web service);
    // clang-format on

    nil_service_standalone nil_service_create_udp_client(
        const char* host,
        uint16_t port,
        uint64_t buffer,
        double timeout_s
    );
    nil_service_standalone nil_service_create_udp_server(
        const char* host,
        uint16_t port,
        uint64_t buffer,
        double timeout_s
    );

    nil_service_standalone nil_service_create_tcp_client(
        const char* host,
        uint16_t port,
        uint64_t buffer
    );
    nil_service_standalone nil_service_create_tcp_server(
        const char* host,
        uint16_t port,
        uint64_t buffer
    );

#if defined(__unix__) || defined(__unix) || defined(unix) || defined(__APPLE__)
    // Opens an existing FIFO or creates it first. Returns an open fd or -1 on failure.
    int nil_service_pipe_mkfifo(const char* path);
    int nil_service_pipe_r_mkfifo(const char* path);
    int nil_service_pipe_w_mkfifo(const char* path);

    // NOLINTNEXTLINE(modernize-use-using)
    typedef struct nil_service_pipe_fd_provider
    {
        int (*exec)(void*);
        void* context;
        void (*cleanup)(void*);
    } nil_service_pipe_fd_provider;

    // Creates a standalone pipe service from caller-provided fd providers.
    // Providers may return:
    // -1: disable that direction
    // -2: (read provider only) retry later
    // >=0: owned fd transferred to service ownership
    // Provider cleanup callbacks run when provider state is released by the service.
    nil_service_standalone nil_service_create_pipe(
        nil_service_pipe_fd_provider read_fd,
        nil_service_pipe_fd_provider write_fd,
        uint64_t buffer
    );
#endif

    nil_service_standalone nil_service_create_ws_client(
        const char* host,
        uint16_t port,
        const char* route,
        uint64_t buffer
    );
    nil_service_standalone nil_service_create_ws_server(
        const char* host,
        uint16_t port,
        const char* route,
        uint64_t buffer
    );

    nil_service_web nil_service_create_http_server(
        const char* host,
        uint16_t port,
        uint64_t buffer
    );

    nil_service_gateway nil_service_create_gateway(void);

    // Destroy owning handles created by nil_service_create_*.
    // Any converted/view handles derived from these become invalid after destroy.
    void nil_service_gateway_destroy(nil_service_gateway gateway);
    void nil_service_web_destroy(nil_service_web service);
    void nil_service_standalone_destroy(nil_service_standalone service);
#ifdef __cplusplus
}
#endif
