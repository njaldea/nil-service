#pragma once

#include <nil/xalt/MACROS.h>

// NOLINTNEXTLINE(hicpp-deprecated-headers,modernize-deprecated-headers)
#include <stddef.h>

// NOLINTNEXTLINE(hicpp-deprecated-headers,modernize-deprecated-headers)
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif
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
    } nil_service_standalone; // IStandalone: IEventService, IEventService

    // NOLINTNEXTLINE(modernize-use-using)
    typedef struct nil_service_gateway
    {
        void* handle;
    } nil_service_gateway; // IGatewayService

    void nil_service_gateway_add_service(nil_service_gateway gateway, nil_service_event service);

    // NOLINTNEXTLINE(modernize-use-using)
    typedef struct nil_service_dispatch_info
    {
        void (*exec)(void*);
        void* context;
        void (*cleanup)(void*);
    } nil_service_dispatch_info;

    void nil_service_runnable_start(nil_service_runnable service);
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
        void* _; // std::string (*to_string)(const void*);
    } nil_service_id;

    void nil_service_id_print(nil_service_id id);

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
        nil_service_id id,
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

    // clang-format off
    nil_service_message nil_service_event_to_message(nil_service_event service);
    nil_service_callback nil_service_event_to_callback(nil_service_event service);
    
    nil_service_standalone nil_service_gateway_to_standalone(nil_service_gateway service);
    nil_service_event nil_service_standalone_to_event(nil_service_standalone service);
    nil_service_runnable nil_service_standalone_to_runnable(nil_service_standalone service);
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

    nil_service_gateway nil_service_create_gateway(void);
    void nil_service_gateway_destroy(nil_service_gateway gateway);
    void nil_service_standalone_destroy(nil_service_standalone service);
#ifdef __cplusplus
}
#endif
