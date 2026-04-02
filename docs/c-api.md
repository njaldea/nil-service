# nil/service C API

This document covers the C API exposed by nil/service.

Header:
- nil/service.h

Build:
- Enable `ENABLE_C_API` to build `service-c-api`.

## Scope Boundaries

Supported creators in the C API:
- tcp client/server
- udp client/server
- ws client/server
- http server
- gateway

Not currently exposed in the C API:
- self creator

## Handle Model

All C API handles are opaque wrappers around C++ interface pointers.

Owning handles (must be destroyed):
- nil_service_standalone (from nil_service_create_* for tcp/udp/ws)
- nil_service_web (from nil_service_create_http_server)
- nil_service_gateway (from nil_service_create_gateway)

View handles (non-owning):
- nil_service_runnable
- nil_service_message
- nil_service_callback
- nil_service_event
- nil_service_web_transaction

Rule:
- Conversion helpers return non-owning views.
- Destroy only the original owning handle.

Precondition rule:
- Unless otherwise documented, API functions expect valid non-null handles.
- Callback registration expects valid function pointers in callback structs.

## Creation And Destruction

Standalone creators:
- nil_service_create_udp_client
- nil_service_create_udp_server
- nil_service_create_tcp_client
- nil_service_create_tcp_server
- nil_service_create_ws_client
- nil_service_create_ws_server

Web creator:
- nil_service_create_http_server

Defaults:
- timeout passed to udp create APIs: caller provided
- buffer passed to create APIs: caller provided
- ws route defaults to `/` when route argument is null

Gateway creator:
- nil_service_create_gateway

Destroy:
- nil_service_standalone_destroy
- nil_service_web_destroy
- nil_service_gateway_destroy

## Core Runnable API

Use runnable views to run services:
- nil_service_runnable_start
- nil_service_runnable_stop
- nil_service_runnable_restart
- nil_service_runnable_dispatch

## Messaging API

Send/publish through a message view:
- nil_service_message_publish
- nil_service_message_publish_ex
- nil_service_message_send
- nil_service_message_publish_ex_ids
- nil_service_message_send_ids

## Callback API

Register callbacks through a callback view:
- nil_service_callback_on_ready
- nil_service_callback_on_connect
- nil_service_callback_on_disconnect
- nil_service_callback_on_message

## Web API

Web callbacks:
- nil_service_web_on_ready
- nil_service_web_on_get

Web route helpers:
- nil_service_web_transaction_set_content_type
- nil_service_web_transaction_send
- nil_service_web_transaction_get_route

Websocket integration:
- nil_service_web_use_ws (returns nil_service_event for the websocket route service)

## Conversion Helpers

Event conversions:
- nil_service_event_to_message
- nil_service_event_to_callback

Gateway conversions:
- nil_service_gateway_to_standalone
- nil_service_gateway_to_event
- nil_service_gateway_to_message
- nil_service_gateway_to_callback
- nil_service_gateway_to_runnable

Standalone conversions:
- nil_service_standalone_to_event
- nil_service_standalone_to_message
- nil_service_standalone_to_callback
- nil_service_standalone_to_runnable

Web conversions:
- nil_service_web_to_runnable

## Lifetime And Thread-Safety

nil_service_web_transaction:
- Valid only during nil_service_web_on_get callback execution.
- nil_service_web_transaction_get_route returns a non-null-terminated view.
- Route pointer and transaction handle must not be stored after callback returns.
- nil_service_web_transaction_send copies response payload before return.

Handle preconditions:
- Most functions do not perform runtime null checks and assume valid handles.
- Passing invalid/null handles is undefined behavior.

nil_service_id:
- nil_service_id_print (string conversion) is valid only during the callback that provided the id.
- String conversion is not thread-safe.

## Minimal Usage Pattern

1. Create owning service handle.
2. Convert to views (runnable/message/callback/event) as needed.
3. Register callbacks.
4. Start runnable.
5. Stop runnable.
6. Destroy owning handle.
