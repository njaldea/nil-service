from __future__ import annotations

import sys
import time
import argparse
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[1]
PY_FFI_DIR = REPO_ROOT / "src" / "ffi" / "python"

print(PY_FFI_DIR)
sys.path.insert(0, str(PY_FFI_DIR))

import nil_service

def setup_handlers(service: nil_service.Event):
    """Setup event handlers for a service."""

    def on_ready(id_obj):
        print(f"local        : {id_obj.to_string()}", flush=True)

    def on_connect(id_obj):
        print(f"connected    : {id_obj.to_string()}", flush=True)

    def on_disconnect(id_obj):
        print(f"disconnected : {id_obj.to_string()}", flush=True)

    def on_message(id_obj, data):
        print(f"from         : {id_obj.to_string()}", flush=True)
        print(f"type         : {data[:1].decode('utf-8', errors='replace')}", flush=True)
        print(f"message      : {data[1:].decode('utf-8', errors='replace')}", flush=True)

    service.on_ready(on_ready)
    service.on_connect(on_connect)
    service.on_disconnect(on_disconnect)
    service.on_message(on_message)


def input_output(service: nil_service.Standalone):
    """Run interactive input/output loop."""
    type_flag = 0

    print("> ", end="", flush=True)

    try:
        while True:
            # Poll service to process events
            service.poll()

            # Try to read from stdin (non-blocking via select)
            import select

            ready, _, _ = select.select([sys.stdin], [], [], 0.001)
            if ready:
                try:
                    line = input()
                except EOFError:
                    break

                if line == "reconnect":
                    break

                prefix = "A" if type_flag == 0 else "B"
                full_message = f"{prefix}typed > {line} : secondary here".encode("utf-8")
                service.publish(full_message)
                type_flag = (type_flag + 1) % 2

                print("> ", end="", flush=True)
            else:
                time.sleep(0.001)  # 1ms sleep
    except KeyboardInterrupt:
        print()
        pass


def run_tcp_server(port):
    """Run a TCP server."""
    print(f"Starting TCP server on port {port}")
    service = nil_service.create_tcp_server("127.0.0.1", port, 1024 * 1024)
    setup_handlers(service)
    print("Running TCP server... (type messages or 'reconnect' to exit)")
    input_output(service)
    service.stop()
    service.destroy()


def run_udp_server(port):
    """Run a UDP server."""
    print(f"Starting UDP server on port {port}")
    service = nil_service.create_udp_server("127.0.0.1", port, 1024 * 1024)
    setup_handlers(service)
    print("Running UDP server... (type messages or 'reconnect' to exit)")
    input_output(service)
    service.stop()
    service.destroy()


def run_ws_server(port, route):
    """Run a WebSocket server."""
    print(f"Starting WebSocket server on port {port} at {route}")
    service = nil_service.create_ws_server("127.0.0.1", port, route, 1024 * 1024)
    setup_handlers(service)
    print("Running WebSocket server... (type messages or 'reconnect' to exit)")
    input_output(service)
    service.stop()
    service.destroy()


def run_gateway():
    """Run a gateway with TCP, UDP, and WebSocket services."""
    print("Starting Gateway with TCP, UDP, and WebSocket services")

    tcp = nil_service.create_tcp_server("127.0.0.1", 9000, 1024 * 1024)
    udp = nil_service.create_udp_server("127.0.0.1", 9001, 1024 * 1024)
    ws = nil_service.create_ws_server("127.0.0.1", 9002, "/", 1024 * 1024)

    gateway = nil_service.create_gateway()

    # Add all services to gateway
    gateway.add_service(tcp)
    gateway.add_service(udp)
    gateway.add_service(ws)

    # Setup handlers on gateway
    setup_handlers(gateway)

    print("Running Gateway... (type messages or 'reconnect' to exit)")
    input_output(gateway)

    gateway.stop()

    tcp.destroy()
    udp.destroy()
    ws.destroy()
    gateway.destroy()


def run_tcp_client(port):
    """Run a TCP client."""
    print(f"Starting TCP client connecting to port {port}")
    service = nil_service.create_tcp_client("127.0.0.1", port, 1024 * 1024)
    setup_handlers(service)
    print("Running TCP client... (type messages or 'reconnect' to exit)")
    input_output(service)
    service.stop()
    service.destroy()


def run_udp_client(port):
    """Run a UDP client."""
    print(f"Starting UDP client connecting to port {port}")
    service = nil_service.create_udp_client("127.0.0.1", port, 1024 * 1024)
    setup_handlers(service)
    print("Running UDP client... (type messages or 'reconnect' to exit)")
    input_output(service)
    service.stop()
    service.destroy()


def run_ws_client(port, route):
    """Run a WebSocket client."""
    print(f"Starting WebSocket client connecting to port {port} at {route}")
    service = nil_service.create_ws_client("127.0.0.1", port, route, 1024 * 1024)
    setup_handlers(service)
    print("Running WebSocket client... (type messages or 'reconnect' to exit)")
    input_output(service)
    service.stop()
    service.destroy()


def run_http_server(port):
    """Run an HTTP server."""
    print(f"Starting HTTP server on port {port}")

    web = nil_service.create_http_server("0.0.0.0", port, 100 * 1024 * 1024)

    def on_ready(id_obj: nil_service.ID):
        print(f"HTTP server ready: {id_obj.to_string()}")

    def on_get(transaction: nil_service.WebTransaction):
        route = transaction.get_route()
        if route == "/":
            transaction.set_content_type("text/html")
            transaction.send(
                b"<!DOCTYPE html>"
                b"<html lang=\"en\">"
                b"<head>"
                b"<title>nil-service HTTP</title>"
                b"</head>"
                b"<body><h1>Hello from nil-service Python FFI</h1></body>"
                b"</html>"
            )
            return True
        return False

    web.on_ready(on_ready)
    web.on_get(on_get)

    # Setup handlers on the WebSocket event
    ws_event = web.use_ws("/ws")
    setup_handlers(ws_event)

    print("Running HTTP server... (press Ctrl+C to exit)")

    # Simple polling loop
    try:
        for _ in range(100):
            web.poll()
            time.sleep(0.1)  # 100ms sleep
    except KeyboardInterrupt:
        print()

    web.destroy()


def print_help():
    """Print help message."""
    print("Usage: python main.py <command> [options]")
    print("")
    print("Server Commands:")
    print("  tcp-server [port]       Run TCP server (default port: 9000)")
    print("  udp-server [port]       Run UDP server (default port: 9001)")
    print("  ws-server [port] [route] Run WebSocket server (default: 9002, /)")
    print("  http-server [port]      Run HTTP server (default port: 8080)")
    print("  gateway                 Run Gateway with all protocols")
    print("")
    print("Client Commands:")
    print("  tcp-client [port]       Run TCP client (default port: 9000)")
    print("  udp-client [port]       Run UDP client (default port: 9001)")
    print("  ws-client [port] [route] Run WebSocket client (default: 9002, /)")
    print("")
    print("Other Commands:")
    print("  help                    Show this help message")


def main():
    """Main entry point."""
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument("command", nargs="?", default="help")
    parser.add_argument("args", nargs="*")

    parsed = parser.parse_args()
    command = parsed.command
    args = parsed.args

    if command == "tcp-server":
        port = int(args[0]) if args else 9000
        run_tcp_server(port)
    elif command == "tcp-client":
        port = int(args[0]) if args else 9000
        run_tcp_client(port)
    elif command == "udp-server":
        port = int(args[0]) if args else 9001
        run_udp_server(port)
    elif command == "udp-client":
        port = int(args[0]) if args else 9001
        run_udp_client(port)
    elif command == "ws-server":
        port = int(args[0]) if args else 9002
        route = args[1] if len(args) > 1 else "/"
        run_ws_server(port, route)
    elif command == "ws-client":
        port = int(args[0]) if args else 9002
        route = args[1] if len(args) > 1 else "/"
        run_ws_client(port, route)
    elif command == "gateway":
        run_gateway()
    elif command == "http-server":
        port = int(args[0]) if args else 8080
        run_http_server(port)
    else:
        print_help()


if __name__ == "__main__":
    main()
