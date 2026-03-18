#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Opaque handle for a service instance.
     *  Obtained from one of the nil_service_*_create functions.
     *  Must be released with nil_service_free.
     */
    typedef void* NilServiceHandle;

    /**
     * @brief Temporary identifier for a connection.
     *  Valid only for the duration of the callback invocation.
     */
    typedef struct
    {
        const char* text;
    } NilServiceID;

    /** Callback invoked when the service is ready. */
    typedef void (*NilServiceReadyHandler)(const NilServiceID* id, void* user_data);

    /** Callback invoked when a client connects. */
    typedef void (*NilServiceConnectHandler)(const NilServiceID* id, void* user_data);

    /** Callback invoked when a client disconnects. */
    typedef void (*NilServiceDisconnectHandler)(const NilServiceID* id, void* user_data);

    /** Callback invoked when a message is received. */
    typedef void (*NilServiceMessageHandler)(
        const NilServiceID* id,
        const void* data,
        uint64_t size,
        void* user_data
    );

    /** Callback invoked in the service thread via nil_service_dispatch. */
    typedef void (*NilServiceTask)(void* user_data);

    /* ------------------------------------------------------------------ */
    /*  TCP service options                                                 */
    /* ------------------------------------------------------------------ */

    typedef struct
    {
        const char* host;
        uint16_t port;
        /** Buffer size for receiving. Set to 0 to use the default (1024). */
        uint64_t buffer;
    } NilServiceTcpOptions;

    /* ------------------------------------------------------------------ */
    /*  UDP service options                                                 */
    /* ------------------------------------------------------------------ */

    typedef struct
    {
        const char* host;
        uint16_t port;
        /** Buffer size for receiving. Set to 0 to use the default (1024). */
        uint64_t buffer;
        /** Inactivity timeout in nanoseconds. Set to 0 to use the default (2 s). */
        uint64_t timeout_ns;
    } NilServiceUdpOptions;

    /* ------------------------------------------------------------------ */
    /*  WebSocket service options                                           */
    /* ------------------------------------------------------------------ */

    typedef struct
    {
        const char* host;
        uint16_t port;
        /** HTTP upgrade route. NULL or empty string uses the default ("/"). */
        const char* route;
        /** Buffer size for receiving. Set to 0 to use the default (1024). */
        uint64_t buffer;
    } NilServiceWsOptions;

    /* ------------------------------------------------------------------ */
    /*  Service creation                                                    */
    /* ------------------------------------------------------------------ */

    NilServiceHandle nil_service_tcp_server_create(const NilServiceTcpOptions* options);
    NilServiceHandle nil_service_tcp_client_create(const NilServiceTcpOptions* options);

    NilServiceHandle nil_service_udp_server_create(const NilServiceUdpOptions* options);
    NilServiceHandle nil_service_udp_client_create(const NilServiceUdpOptions* options);

    NilServiceHandle nil_service_ws_server_create(const NilServiceWsOptions* options);
    NilServiceHandle nil_service_ws_client_create(const NilServiceWsOptions* options);

    NilServiceHandle nil_service_self_create(void);
    NilServiceHandle nil_service_gateway_create(void);

    /* ------------------------------------------------------------------ */
    /*  Service lifecycle                                                   */
    /* ------------------------------------------------------------------ */

    /** Destroy the service and free all associated resources. */
    void nil_service_free(NilServiceHandle handle);

    /** Start the service. Blocks until nil_service_stop is called. */
    void nil_service_start(NilServiceHandle handle);

    /** Stop the service. Non-blocking. */
    void nil_service_stop(NilServiceHandle handle);

    /**
     * @brief Prepare the service for reuse after stopping.
     *  Must be called before calling nil_service_start again.
     */
    void nil_service_restart(NilServiceHandle handle);

    /** Execute a task in the service thread. */
    void nil_service_dispatch(NilServiceHandle handle, NilServiceTask task, void* user_data);

    /* ------------------------------------------------------------------ */
    /*  Event handlers                                                      */
    /* ------------------------------------------------------------------ */

    void nil_service_on_ready(
        NilServiceHandle handle,
        NilServiceReadyHandler handler,
        void* user_data
    );

    void nil_service_on_connect(
        NilServiceHandle handle,
        NilServiceConnectHandler handler,
        void* user_data
    );

    void nil_service_on_disconnect(
        NilServiceHandle handle,
        NilServiceDisconnectHandler handler,
        void* user_data
    );

    void nil_service_on_message(
        NilServiceHandle handle,
        NilServiceMessageHandler handler,
        void* user_data
    );

    /* ------------------------------------------------------------------ */
    /*  Messaging                                                           */
    /* ------------------------------------------------------------------ */

    /** Broadcast a message to all connected clients. */
    void nil_service_publish(NilServiceHandle handle, const void* data, uint64_t size);

    /**
     * @brief Broadcast a message to all connected clients except those listed.
     *
     * @param id_texts  array of connection id text strings to exclude
     * @param id_count  number of entries in id_texts
     */
    void nil_service_publish_ex(
        NilServiceHandle handle,
        const char* const* id_texts,
        uint64_t id_count,
        const void* data,
        uint64_t size
    );

    /** Send a message to a single connection identified by id_text. */
    void nil_service_send(
        NilServiceHandle handle,
        const char* id_text,
        const void* data,
        uint64_t size
    );

    /* ------------------------------------------------------------------ */
    /*  Gateway                                                             */
    /* ------------------------------------------------------------------ */

    /**
     * @brief Register a service with a gateway.
     *  The service handle must remain valid for as long as the gateway may
     *  invoke it (i.e. until after nil_service_stop is called on the gateway
     *  or nil_service_free is called on the gateway).
     *
     * @param gateway  handle returned by nil_service_gateway_create
     * @param service  handle for the service to add
     */
    void nil_service_gateway_add(NilServiceHandle gateway, NilServiceHandle service);

#ifdef __cplusplus
}
#endif
