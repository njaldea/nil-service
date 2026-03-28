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
    struct nil_service_runnable; // IRunnable

    struct nil_service_message; // IMessageService

    struct nil_service_callback; // ICallbackService

    struct nil_service_event; // IEventService: IMessageService, ICallbackService

    struct nil_service_standalone; // IStandalone: IEventService, IEventService

    struct nil_service_dispatch_info
    {
        void (*exec)(void*);
        void* context;
        void (*cleanup)(void*);
    };

    void nil_service_runnable_start(struct nil_service_runnable* service);
    void nil_service_runnable_stop(struct nil_service_runnable* service);
    void nil_service_runnable_restart(struct nil_service_runnable* service);
    void nil_service_runnable_dispatch(
        struct nil_service_runnable* service,
        struct nil_service_dispatch_info callback
    );

    struct nil_service_id;

    struct nil_service_ids
    {
        uint32_t size;
        nil_service_id* ids;
    };

    void nil_service_message_publish(
        struct nil_service_message* service,
        const char* data,
        uint64_t size
    );
    void nil_service_message_publish_ex(
        struct nil_service_id* id,
        struct nil_service_message* service,
        const void* data,
        uint64_t size
    );
    void nil_service_message_send(
        struct nil_service_id* id,
        struct nil_service_message* service,
        const void* data,
        uint64_t size
    );

    void nil_service_message_publish_ex_ids(
        struct nil_service_ids* ids,
        struct nil_service_message* service,
        const void* data,
        uint64_t size
    );
    void nil_service_message_send_ids(
        struct nil_service_id* id,
        struct nil_service_message* service,
        const void* data,
        uint64_t size
    );

    struct nil_service_msg_callback_info
    {
        void (*exec)(struct nil_service_id*, const void*, uint64_t, void*);
        void* context;
        void (*cleanup)(void*);
    };

    struct nil_service_callback_info
    {
        void (*exec)(struct nil_service_id* id, void*);
        void* context;
        void (*cleanup)(void*);
    };

    void nil_service_callback_on_ready(
        struct nil_service_callback* service,
        nil_service_callback_info callback
    );

    void nil_service_callback_on_connect(
        struct nil_service_callback* service,
        nil_service_callback_info callback
    );

    void nil_service_callback_on_disconnect(
        struct nil_service_callback* service,
        nil_service_callback_info callback
    );

    void nil_service_callback_on_message(
        struct nil_service_callback* service,
        nil_service_msg_callback_info callback
    );

    // clang-format off
    struct nil_service_message* nil_service_event_to_message(struct nil_service_event* service);
    struct nil_service_callback* nil_service_event_to_callback(struct nil_service_event* service);
    
    struct nil_service_event* nil_service_standalone_to_event(struct nil_service_standalone* service);
    struct nil_service_runnable* nil_service_standalone_to_runnable(struct nil_service_standalone* service);
    // clang-format on
#ifdef __cplusplus
}
#endif
