from __future__ import annotations

import ctypes
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Callable, Dict, Optional, List


class nil_serviceRunnable(ctypes.Structure):
    _fields_ = [("handle", ctypes.c_void_p)]


class nil_serviceMessage(ctypes.Structure):
    _fields_ = [("handle", ctypes.c_void_p)]


class nil_serviceCallback(ctypes.Structure):
    _fields_ = [("handle", ctypes.c_void_p)]


class nil_serviceEvent(ctypes.Structure):
    _fields_ = [("handle", ctypes.c_void_p)]


class nil_serviceStandalone(ctypes.Structure):
    _fields_ = [("handle", ctypes.c_void_p)]


class nil_serviceGateway(ctypes.Structure):
    _fields_ = [("handle", ctypes.c_void_p)]


class nil_serviceWeb(ctypes.Structure):
    _fields_ = [("handle", ctypes.c_void_p)]


class nil_serviceWebTransaction(ctypes.Structure):
    _fields_ = [("handle", ctypes.c_void_p)]


class nil_serviceId(ctypes.Structure):
    _fields_ = [
        ("owner", ctypes.c_void_p),
        ("id", ctypes.c_void_p),
        ("_", ctypes.c_void_p),
    ]


class nil_serviceIds(ctypes.Structure):
    _fields_ = [
        ("size", ctypes.c_uint32),
        ("ids", ctypes.POINTER(nil_serviceId)),
    ]


# Callback function types
NIL_SERVICE_CALLBACK_EXEC = ctypes.CFUNCTYPE(
    None, ctypes.POINTER(nil_serviceId), ctypes.c_void_p
)
NIL_SERVICE_MSG_CALLBACK_EXEC = ctypes.CFUNCTYPE(
    None, ctypes.POINTER(nil_serviceId), ctypes.c_void_p, ctypes.c_uint64, ctypes.c_void_p
)
NIL_SERVICE_DISPATCH_EXEC = ctypes.CFUNCTYPE(None, ctypes.c_void_p)
NIL_SERVICE_WEB_GET_CALLBACK_EXEC = ctypes.CFUNCTYPE(
    ctypes.c_int, nil_serviceWebTransaction, ctypes.c_void_p
)
NIL_SERVICE_CLEANUP = ctypes.CFUNCTYPE(None, ctypes.c_void_p)


class nil_serviceDispatchInfo(ctypes.Structure):
    _fields_ = [
        ("exec", NIL_SERVICE_DISPATCH_EXEC),
        ("context", ctypes.c_void_p),
        ("cleanup", NIL_SERVICE_CLEANUP),
    ]


class nil_serviceMsgCallbackInfo(ctypes.Structure):
    _fields_ = [
        ("exec", NIL_SERVICE_MSG_CALLBACK_EXEC),
        ("context", ctypes.c_void_p),
        ("cleanup", NIL_SERVICE_CLEANUP),
    ]


class nil_serviceCallbackInfo(ctypes.Structure):
    _fields_ = [
        ("exec", NIL_SERVICE_CALLBACK_EXEC),
        ("context", ctypes.c_void_p),
        ("cleanup", NIL_SERVICE_CLEANUP),
    ]


class nil_serviceWebGetCallbackInfo(ctypes.Structure):
    _fields_ = [
        ("exec", NIL_SERVICE_WEB_GET_CALLBACK_EXEC),
        ("context", ctypes.c_void_p),
        ("cleanup", NIL_SERVICE_CLEANUP),
    ]


def _to_ref_id(ptr: Any) -> int:
    """Convert a ctypes pointer to an integer reference ID."""
    if ptr is None:
        return 0
    return int(ctypes.cast(ptr, ctypes.c_void_p).value)


@dataclass
class _CallbackState:
    fn: Callable[[Any], Any]


class ID:
    """Wraps a nil_service_id with convenience methods."""

    def __init__(self, c_id: nil_serviceId, lib: Any) -> None:
        self._c_id = c_id
        self._lib = lib

    def to_string(self) -> str:
        buf = ctypes.create_string_buffer(256)
        n = self._lib.nil_service_id_to_string(self._c_id, buf, 256)
        return buf.raw[:n].decode("utf-8", errors="replace")


class Message:
    """Message service wrapper."""

    def __init__(
        self, message: nil_serviceMessage, lib: Any, refs: Dict[int, Any]
    ) -> None:
        self._message = message
        self._lib = lib
        self._refs = refs

    def publish(self, data: bytes) -> None:
        """Publish data to all subscribers."""
        self._lib.nil_service_message_publish(
            self._message, ctypes.c_char_p(data), len(data)
        )

    def publish_ex(self, id_obj: ID, data: bytes) -> None:
        """Publish data to a specific ID."""
        self._lib.nil_service_message_publish_ex(
            id_obj._c_id, self._message, ctypes.c_char_p(data), len(data)
        )

    def send(self, id_obj: ID, data: bytes) -> None:
        """Send data to a specific ID."""
        self._lib.nil_service_message_send(
            id_obj._c_id, self._message, ctypes.c_char_p(data), len(data)
        )


class Callback:
    """Callback service wrapper."""

    def __init__(
        self, callback: nil_serviceCallback, lib: Any, fns: Any, refs: Dict[int, Any]
    ) -> None:
        self._callback = callback
        self._lib = lib
        self._fns = fns
        self._refs = refs

    def on_ready(self, fn: Callable[[ID], None]) -> None:
        """Register on_ready callback."""
        callback_info = nil_serviceCallbackInfo(
            exec=self._fns["callback_exec"],
            context=self._fns["store_callback"](fn),
            cleanup=self._fns["cleanup"],
        )
        self._lib.nil_service_callback_on_ready(self._callback, callback_info)

    def on_connect(self, fn: Callable[[ID], None]) -> None:
        """Register on_connect callback."""
        callback_info = nil_serviceCallbackInfo(
            exec=self._fns["callback_exec"],
            context=self._fns["store_callback"](fn),
            cleanup=self._fns["cleanup"],
        )
        self._lib.nil_service_callback_on_connect(self._callback, callback_info)

    def on_disconnect(self, fn: Callable[[ID], None]) -> None:
        """Register on_disconnect callback."""
        callback_info = nil_serviceCallbackInfo(
            exec=self._fns["callback_exec"],
            context=self._fns["store_callback"](fn),
            cleanup=self._fns["cleanup"],
        )
        self._lib.nil_service_callback_on_disconnect(self._callback, callback_info)

    def on_message(self, fn: Callable[[ID, bytes], None]) -> None:
        """Register on_message callback."""
        callback_info = nil_serviceMsgCallbackInfo(
            exec=self._fns["msg_callback_exec"],
            context=self._fns["store_callback"](fn),
            cleanup=self._fns["cleanup"],
        )
        self._lib.nil_service_callback_on_message(self._callback, callback_info)


class Event(Message, Callback):
    """Event service wraps both Message and Callback functionality."""

    def __init__(
        self,
        event: nil_serviceEvent,
        lib: Any,
        fns: Any,
        refs: Dict[int, Any],
    ) -> None:
        self._event = event
        message = lib.nil_service_event_to_message(event)
        callback = lib.nil_service_event_to_callback(event)
        Message.__init__(self, message, lib, refs)
        Callback.__init__(self, callback, lib, fns, refs)


class Runnable:
    """Runnable service wrapper."""

    def __init__(
        self, runnable: nil_serviceRunnable, lib: Any, fns: Any, refs: Dict[int, Any]
    ) -> None:
        self._runnable = runnable
        self._lib = lib
        self._fns = fns
        self._refs = refs

    def run(self) -> None:
        """Run the service."""
        self._lib.nil_service_runnable_run(self._runnable)

    def poll(self) -> None:
        """Poll the service once."""
        self._lib.nil_service_runnable_poll(self._runnable)

    def stop(self) -> None:
        """Stop the service."""
        self._lib.nil_service_runnable_stop(self._runnable)

    def restart(self) -> None:
        """Restart the service."""
        self._lib.nil_service_runnable_restart(self._runnable)

    def dispatch(self, fn: Callable[[], None]) -> None:
        """Dispatch a callback."""
        dispatch_info = nil_serviceDispatchInfo(
            exec=self._fns["dispatch_exec"],
            context=self._fns["store_callback"](fn),
            cleanup=self._fns["cleanup"],
        )
        self._lib.nil_service_runnable_dispatch(self._runnable, dispatch_info)


class WebTransaction:
    """Web transaction wrapper."""

    def __init__(self, transaction: nil_serviceWebTransaction, lib: Any) -> None:
        self._transaction = transaction
        self._lib = lib

    def set_content_type(self, content_type: str) -> None:
        """Set the response content type."""
        self._lib.nil_service_web_transaction_set_content_type(
            self._transaction, content_type.encode("utf-8")
        )

    def send(self, data: bytes) -> None:
        """Send response body."""
        self._lib.nil_service_web_transaction_send(
            self._transaction, ctypes.c_char_p(data), len(data)
        )

    def get_route(self) -> str:
        """Get the request route."""
        size = ctypes.c_uint64()
        route_ptr = self._lib.nil_service_web_transaction_get_route(
            self._transaction, ctypes.byref(size)
        )
        return ctypes.string_at(route_ptr, size.value).decode("utf-8", errors="replace")


class Web(Runnable):
    """Web service wrapper."""

    def __init__(
        self, web: nil_serviceWeb, lib: Any, fns: Any, refs: Dict[int, Any]
    ) -> None:
        self._web = web
        runnable = lib.nil_service_web_to_runnable(web)
        Runnable.__init__(self, runnable, lib, fns, refs)

    def on_ready(self, fn: Callable[[ID], None]) -> None:
        """Register on_ready callback."""
        callback_info = nil_serviceCallbackInfo(
            exec=self._fns["callback_exec"],
            context=self._fns["store_callback"](fn),
            cleanup=self._fns["cleanup"],
        )
        self._lib.nil_service_web_on_ready(self._web, callback_info)

    def on_get(self, fn: Callable[[WebTransaction], bool]) -> None:
        """Register on_get callback."""
        callback_info = nil_serviceWebGetCallbackInfo(
            exec=self._fns["web_get_callback_exec"],
            context=self._fns["store_callback"](fn),
            cleanup=self._fns["cleanup"],
        )
        self._lib.nil_service_web_on_get(self._web, callback_info)

    def use_ws(self, route: str) -> Event:
        """Create a WebSocket event on the given route."""
        event = self._lib.nil_service_web_use_ws(self._web, route.encode("utf-8"))
        return Event(event, self._lib, self._fns, self._refs)

    def destroy(self) -> None:
        """Destroy the web service."""
        self._lib.nil_service_web_destroy(self._web)


class Standalone(Event, Runnable):
    """Standalone service wraps Event and Runnable."""

    def __init__(
        self,
        standalone: nil_serviceStandalone,
        lib: Any,
        fns: Any,
        refs: Dict[int, Any],
    ) -> None:
        self._standalone = standalone
        message = lib.nil_service_standalone_to_message(standalone)
        callback = lib.nil_service_standalone_to_callback(standalone)
        runnable = lib.nil_service_standalone_to_runnable(standalone)
        Message.__init__(self, message, lib, refs)
        Callback.__init__(self, callback, lib, fns, refs)
        Runnable.__init__(self, runnable, lib, fns, refs)

    def destroy(self) -> None:
        """Destroy the standalone service."""
        self._lib.nil_service_standalone_destroy(self._standalone)


class Gateway(Standalone):
    """Gateway service wrapper."""

    def __init__(
        self,
        gateway: nil_serviceGateway,
        lib: Any,
        fns: Any,
        refs: Dict[int, Any],
    ) -> None:
        self._gateway = gateway
        message = lib.nil_service_gateway_to_message(gateway)
        callback = lib.nil_service_gateway_to_callback(gateway)
        runnable = lib.nil_service_gateway_to_runnable(gateway)
        Message.__init__(self, message, lib, refs)
        Callback.__init__(self, callback, lib, fns, refs)
        Runnable.__init__(self, runnable, lib, fns, refs)

    def add_service(self, service: Standalone) -> None:
        """Add a service to the gateway."""
        event = self._lib.nil_service_standalone_to_event(service._standalone)
        self._lib.nil_service_gateway_add_service(self._gateway, event)

    def destroy(self) -> None:
        """Destroy the gateway."""
        self._lib.nil_service_gateway_destroy(self._gateway)


def _configure_signatures(lib: Any) -> None:
    """Configure ctypes signatures for all library functions."""
    lib.nil_service_gateway_add_service.argtypes = [
        nil_serviceGateway,
        nil_serviceEvent,
    ]
    lib.nil_service_gateway_add_service.restype = None

    lib.nil_service_runnable_run.argtypes = [nil_serviceRunnable]
    lib.nil_service_runnable_run.restype = None

    lib.nil_service_runnable_poll.argtypes = [nil_serviceRunnable]
    lib.nil_service_runnable_poll.restype = None

    lib.nil_service_runnable_stop.argtypes = [nil_serviceRunnable]
    lib.nil_service_runnable_stop.restype = None

    lib.nil_service_runnable_restart.argtypes = [nil_serviceRunnable]
    lib.nil_service_runnable_restart.restype = None

    lib.nil_service_runnable_dispatch.argtypes = [
        nil_serviceRunnable,
        nil_serviceDispatchInfo,
    ]
    lib.nil_service_runnable_dispatch.restype = None

    lib.nil_service_id_to_string.argtypes = [
        nil_serviceId,
        ctypes.c_char_p,
        ctypes.c_uint64,
    ]
    lib.nil_service_id_to_string.restype = ctypes.c_uint64

    lib.nil_service_message_publish.argtypes = [
        nil_serviceMessage,
        ctypes.c_void_p,
        ctypes.c_uint64,
    ]
    lib.nil_service_message_publish.restype = None

    lib.nil_service_message_publish_ex.argtypes = [
        nil_serviceId,
        nil_serviceMessage,
        ctypes.c_void_p,
        ctypes.c_uint64,
    ]
    lib.nil_service_message_publish_ex.restype = None

    lib.nil_service_message_send.argtypes = [
        nil_serviceId,
        nil_serviceMessage,
        ctypes.c_void_p,
        ctypes.c_uint64,
    ]
    lib.nil_service_message_send.restype = None

    lib.nil_service_message_publish_ex_ids.argtypes = [
        nil_serviceIds,
        nil_serviceMessage,
        ctypes.c_void_p,
        ctypes.c_uint64,
    ]
    lib.nil_service_message_publish_ex_ids.restype = None

    lib.nil_service_message_send_ids.argtypes = [
        nil_serviceIds,
        nil_serviceMessage,
        ctypes.c_void_p,
        ctypes.c_uint64,
    ]
    lib.nil_service_message_send_ids.restype = None

    lib.nil_service_callback_on_ready.argtypes = [
        nil_serviceCallback,
        nil_serviceCallbackInfo,
    ]
    lib.nil_service_callback_on_ready.restype = None

    lib.nil_service_callback_on_connect.argtypes = [
        nil_serviceCallback,
        nil_serviceCallbackInfo,
    ]
    lib.nil_service_callback_on_connect.restype = None

    lib.nil_service_callback_on_disconnect.argtypes = [
        nil_serviceCallback,
        nil_serviceCallbackInfo,
    ]
    lib.nil_service_callback_on_disconnect.restype = None

    lib.nil_service_callback_on_message.argtypes = [
        nil_serviceCallback,
        nil_serviceMsgCallbackInfo,
    ]
    lib.nil_service_callback_on_message.restype = None

    lib.nil_service_web_on_ready.argtypes = [
        nil_serviceWeb,
        nil_serviceCallbackInfo,
    ]
    lib.nil_service_web_on_ready.restype = None

    lib.nil_service_web_on_get.argtypes = [
        nil_serviceWeb,
        nil_serviceWebGetCallbackInfo,
    ]
    lib.nil_service_web_on_get.restype = None

    lib.nil_service_web_use_ws.argtypes = [nil_serviceWeb, ctypes.c_char_p]
    lib.nil_service_web_use_ws.restype = nil_serviceEvent

    lib.nil_service_web_transaction_set_content_type.argtypes = [
        nil_serviceWebTransaction,
        ctypes.c_char_p,
    ]
    lib.nil_service_web_transaction_set_content_type.restype = None

    lib.nil_service_web_transaction_send.argtypes = [
        nil_serviceWebTransaction,
        ctypes.c_void_p,
        ctypes.c_uint64,
    ]
    lib.nil_service_web_transaction_send.restype = None

    lib.nil_service_web_transaction_get_route.argtypes = [
        nil_serviceWebTransaction,
        ctypes.POINTER(ctypes.c_uint64),
    ]
    lib.nil_service_web_transaction_get_route.restype = ctypes.c_char_p

    lib.nil_service_event_to_message.argtypes = [nil_serviceEvent]
    lib.nil_service_event_to_message.restype = nil_serviceMessage

    lib.nil_service_event_to_callback.argtypes = [nil_serviceEvent]
    lib.nil_service_event_to_callback.restype = nil_serviceCallback

    lib.nil_service_gateway_to_event.argtypes = [nil_serviceGateway]
    lib.nil_service_gateway_to_event.restype = nil_serviceEvent

    lib.nil_service_gateway_to_message.argtypes = [nil_serviceGateway]
    lib.nil_service_gateway_to_message.restype = nil_serviceMessage

    lib.nil_service_gateway_to_callback.argtypes = [nil_serviceGateway]
    lib.nil_service_gateway_to_callback.restype = nil_serviceCallback

    lib.nil_service_gateway_to_runnable.argtypes = [nil_serviceGateway]
    lib.nil_service_gateway_to_runnable.restype = nil_serviceRunnable

    lib.nil_service_gateway_to_standalone.argtypes = [nil_serviceGateway]
    lib.nil_service_gateway_to_standalone.restype = nil_serviceStandalone

    lib.nil_service_standalone_to_message.argtypes = [nil_serviceStandalone]
    lib.nil_service_standalone_to_message.restype = nil_serviceMessage

    lib.nil_service_standalone_to_callback.argtypes = [nil_serviceStandalone]
    lib.nil_service_standalone_to_callback.restype = nil_serviceCallback

    lib.nil_service_standalone_to_event.argtypes = [nil_serviceStandalone]
    lib.nil_service_standalone_to_event.restype = nil_serviceEvent

    lib.nil_service_standalone_to_runnable.argtypes = [nil_serviceStandalone]
    lib.nil_service_standalone_to_runnable.restype = nil_serviceRunnable

    lib.nil_service_web_to_runnable.argtypes = [nil_serviceWeb]
    lib.nil_service_web_to_runnable.restype = nil_serviceRunnable

    lib.nil_service_create_udp_client.argtypes = [
        ctypes.c_char_p,
        ctypes.c_uint16,
        ctypes.c_uint64,
    ]
    lib.nil_service_create_udp_client.restype = nil_serviceStandalone

    lib.nil_service_create_udp_server.argtypes = [
        ctypes.c_char_p,
        ctypes.c_uint16,
        ctypes.c_uint64,
    ]
    lib.nil_service_create_udp_server.restype = nil_serviceStandalone

    lib.nil_service_create_tcp_client.argtypes = [
        ctypes.c_char_p,
        ctypes.c_uint16,
        ctypes.c_uint64,
    ]
    lib.nil_service_create_tcp_client.restype = nil_serviceStandalone

    lib.nil_service_create_tcp_server.argtypes = [
        ctypes.c_char_p,
        ctypes.c_uint16,
        ctypes.c_uint64,
    ]
    lib.nil_service_create_tcp_server.restype = nil_serviceStandalone

    lib.nil_service_create_ws_client.argtypes = [
        ctypes.c_char_p,
        ctypes.c_uint16,
        ctypes.c_char_p,
        ctypes.c_uint64,
    ]
    lib.nil_service_create_ws_client.restype = nil_serviceStandalone

    lib.nil_service_create_ws_server.argtypes = [
        ctypes.c_char_p,
        ctypes.c_uint16,
        ctypes.c_char_p,
        ctypes.c_uint64,
    ]
    lib.nil_service_create_ws_server.restype = nil_serviceStandalone

    lib.nil_service_create_http_server.argtypes = [
        ctypes.c_char_p,
        ctypes.c_uint16,
        ctypes.c_uint64,
    ]
    lib.nil_service_create_http_server.restype = nil_serviceWeb

    lib.nil_service_create_gateway.argtypes = []
    lib.nil_service_create_gateway.restype = nil_serviceGateway

    lib.nil_service_gateway_destroy.argtypes = [nil_serviceGateway]
    lib.nil_service_gateway_destroy.restype = None

    lib.nil_service_web_destroy.argtypes = [nil_serviceWeb]
    lib.nil_service_web_destroy.restype = None

    lib.nil_service_standalone_destroy.argtypes = [nil_serviceStandalone]
    lib.nil_service_standalone_destroy.restype = None


def _create_lib_fns(refs: Dict[int, Any], lib: Any) -> dict:
    """Create the callback function handlers."""
    
    libc = ctypes.CDLL(None)
    libc.malloc.argtypes = [ctypes.c_size_t]
    libc.malloc.restype = ctypes.c_void_p
    libc.free.argtypes = [ctypes.c_void_p]
    libc.free.restype = None

    @NIL_SERVICE_CALLBACK_EXEC
    def callback_exec(id_ptr: Any, ctx_id: Any) -> None:
        if not ctx_id or not id_ptr:
            return
        ref_id = _to_ref_id(ctx_id)
        if ref_id not in refs:
            return
        callback_data = refs[ref_id]
        id_obj = ID(id_ptr.contents, lib)
        callback_data.fn(id_obj)

    @NIL_SERVICE_MSG_CALLBACK_EXEC
    def msg_callback_exec(id_ptr: Any, data: Any, size: Any, ctx_id: Any) -> None:
        if not ctx_id or not id_ptr or not data:
            return
        ref_id = _to_ref_id(ctx_id)
        if ref_id not in refs:
            return
        callback_data = refs[ref_id]
        id_obj = ID(id_ptr.contents, lib)
        msg_data = ctypes.string_at(data, size)
        callback_data.fn(id_obj, msg_data)

    @NIL_SERVICE_DISPATCH_EXEC
    def dispatch_exec(ctx_id: Any) -> None:
        if not ctx_id:
            return
        ref_id = _to_ref_id(ctx_id)
        if ref_id not in refs:
            return
        callback_data = refs[ref_id]
        callback_data.fn()

    @NIL_SERVICE_WEB_GET_CALLBACK_EXEC
    def web_get_callback_exec(transaction: Any, ctx_id: Any) -> int:
        if not ctx_id:
            return 0
        ref_id = _to_ref_id(ctx_id)
        if ref_id not in refs:
            return 0
        callback_data = refs[ref_id]
        trans_obj = WebTransaction(transaction, lib)
        result = callback_data.fn(trans_obj)
        return 1 if result else 0

    @NIL_SERVICE_CLEANUP
    def cleanup(ctx_id: Any) -> None:
        if ctx_id:
            ref_id = _to_ref_id(ctx_id)
            refs.pop(ref_id, None)
            libc.free(ctx_id)

    def store_callback(fn: Callable[..., Any]) -> Any:
        """Store a callback and return a pointer to it."""
        ctx_id = libc.malloc(1)
        if not ctx_id:
            raise MemoryError("malloc failed")
        refs[_to_ref_id(ctx_id)] = _CallbackState(fn=fn)
        return ctx_id

    return {
        "callback_exec": callback_exec,
        "msg_callback_exec": msg_callback_exec,
        "dispatch_exec": dispatch_exec,
        "web_get_callback_exec": web_get_callback_exec,
        "cleanup": cleanup,
        "store_callback": store_callback,
    }


class Module:
    """Main module for creating nil-service services."""

    def __init__(self) -> None:
        lib_path = Path(__file__).resolve().parent / "libservice-c-api.so"
        self._lib = ctypes.CDLL(str(lib_path))
        _configure_signatures(self._lib)

        self._refs: Dict[int, Any] = {}
        self._fns = _create_lib_fns(self._refs, self._lib)

    def create_gateway(self) -> Gateway:
        """Create a gateway service."""
        gateway = self._lib.nil_service_create_gateway()
        return Gateway(gateway, self._lib, self._fns, self._refs)

    def create_udp_client(
        self, host: str, port: int, buffer: int
    ) -> Standalone:
        """Create a UDP client service."""
        standalone = self._lib.nil_service_create_udp_client(
            host.encode("utf-8"), port, buffer
        )
        return Standalone(standalone, self._lib, self._fns, self._refs)

    def create_udp_server(
        self, host: str, port: int, buffer: int
    ) -> Standalone:
        """Create a UDP server service."""
        standalone = self._lib.nil_service_create_udp_server(
            host.encode("utf-8"), port, buffer
        )
        return Standalone(standalone, self._lib, self._fns, self._refs)

    def create_tcp_client(self, host: str, port: int, buffer: int) -> Standalone:
        """Create a TCP client service."""
        standalone = self._lib.nil_service_create_tcp_client(
            host.encode("utf-8"), port, buffer
        )
        return Standalone(standalone, self._lib, self._fns, self._refs)

    def create_tcp_server(self, host: str, port: int, buffer: int) -> Standalone:
        """Create a TCP server service."""
        standalone = self._lib.nil_service_create_tcp_server(
            host.encode("utf-8"), port, buffer
        )
        return Standalone(standalone, self._lib, self._fns, self._refs)

    def create_ws_client(
        self, host: str, port: int, route: str, buffer: int
    ) -> Standalone:
        """Create a WebSocket client service."""
        standalone = self._lib.nil_service_create_ws_client(
            host.encode("utf-8"), port, route.encode("utf-8"), buffer
        )
        return Standalone(standalone, self._lib, self._fns, self._refs)

    def create_ws_server(
        self, host: str, port: int, route: str, buffer: int
    ) -> Standalone:
        """Create a WebSocket server service."""
        standalone = self._lib.nil_service_create_ws_server(
            host.encode("utf-8"), port, route.encode("utf-8"), buffer
        )
        return Standalone(standalone, self._lib, self._fns, self._refs)

    def create_http_server(self, host: str, port: int, buffer: int) -> Web:
        """Create an HTTP server service."""
        web = self._lib.nil_service_create_http_server(
            host.encode("utf-8"), port, buffer
        )
        return Web(web, self._lib, self._fns, self._refs)


# Global instance
_SERVICE = Module()


def create_gateway() -> Gateway:
    """Create a gateway service."""
    return _SERVICE.create_gateway()


def create_udp_client(
    host: str, port: int, buffer: int
) -> Standalone:
    """Create a UDP client service."""
    return _SERVICE.create_udp_client(host, port, buffer)


def create_udp_server(
    host: str, port: int, buffer: int
) -> Standalone:
    """Create a UDP server service."""
    return _SERVICE.create_udp_server(host, port, buffer)


def create_tcp_client(host: str, port: int, buffer: int) -> Standalone:
    """Create a TCP client service."""
    return _SERVICE.create_tcp_client(host, port, buffer)


def create_tcp_server(host: str, port: int, buffer: int) -> Standalone:
    """Create a TCP server service."""
    return _SERVICE.create_tcp_server(host, port, buffer)


def create_ws_client(host: str, port: int, route: str, buffer: int) -> Standalone:
    """Create a WebSocket client service."""
    return _SERVICE.create_ws_client(host, port, route, buffer)


def create_ws_server(host: str, port: int, route: str, buffer: int) -> Standalone:
    """Create a WebSocket server service."""
    return _SERVICE.create_ws_server(host, port, route, buffer)


def create_http_server(host: str, port: int, buffer: int) -> Web:
    """Create an HTTP server service."""
    return _SERVICE.create_http_server(host, port, buffer)


__all__ = [
    "create_gateway",
    "create_udp_client",
    "create_udp_server",
    "create_tcp_client",
    "create_tcp_server",
    "create_ws_client",
    "create_ws_server",
    "create_http_server",
    "Gateway",
    "Standalone",
    "Web",
    "Event",
    "Message",
    "Callback",
    "WebTransaction",
    "ID",
]
