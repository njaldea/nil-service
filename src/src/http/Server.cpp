#include <boost/asio/strand.hpp>
#include <boost/beast/websocket/rfc6455.hpp>
#include <nil/service/http/Server.hpp>

#include "../utils.hpp"
#include "WebSocketImpl.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/core/ostream.hpp>
#include <boost/beast/http/dynamic_body.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/write.hpp>

#include <iostream>

namespace nil::service::http
{
    struct Route
    {
        std::string content_type;
        std::unique_ptr<detail::ICallable<std::ostream&>> body;
    };

    class Connection final: public std::enable_shared_from_this<Connection>
    {
    public:
        explicit Connection(
            std::unordered_map<std::string, WebSocket>& init_wss,
            const std::unordered_map<std::string, Route>& init_routes,
            std::uint64_t buffer_size,
            boost::asio::ip::tcp::socket init_socket
        )
            : wss(init_wss)
            , routes(init_routes)
            , socket(std::move(init_socket))
            , buffer(buffer_size)
            , deadline(socket.get_executor(), std::chrono::seconds(60))
        {
        }

        void start()
        {
            auto self = shared_from_this();
            boost::beast::http::async_read(
                socket,
                buffer,
                request,
                [self](boost::beast::error_code ec, std::size_t bytes_transferred)
                {
                    (void)bytes_transferred;
                    if (!ec)
                    {
                        self->process_request();
                    }
                }
            );
            deadline.async_wait(
                [self](boost::beast::error_code ec)
                {
                    if (!ec)
                    {
                        self->socket.close(ec);
                    }
                }
            );
        }

    private:
        std::unordered_map<std::string, WebSocket>& wss;      // NOLINT
        const std::unordered_map<std::string, Route>& routes; // NOLINT
        boost::asio::ip::tcp::socket socket;

        boost::beast::flat_buffer buffer;
        boost::beast::http::request<boost::beast::http::dynamic_body> request;
        boost::beast::http::response<boost::beast::http::dynamic_body> response;

        boost::asio::steady_timer deadline;

        void process_request()
        {
            response.version(request.version());
            response.keep_alive(false);

            switch (request.method())
            {
                case boost::beast::http::verb::get:
                {
                    response.result(boost::beast::http::status::ok);
                    response.set(boost::beast::http::field::server, "Beast");

                    {
                        auto it = wss.find(request.target());
                        if (it != wss.end())
                        {
                            if (boost::beast::websocket::is_upgrade(request))
                            {
                                auto id = utils::to_id(socket.remote_endpoint());
                                auto ws = std::make_unique<
                                    boost::beast::websocket::stream<boost::beast::tcp_stream>>(
                                    std::move(socket)
                                );
                                ws->set_option(
                                    boost::beast::websocket::stream_base::timeout::suggested(
                                        boost::beast::role_type::server
                                    )
                                );
                                ws->set_option(boost::beast::websocket::stream_base::decorator(
                                    [](boost::beast::websocket::response_type& res)
                                    {
                                        res.set(
                                            boost::beast::http::field::server,
                                            std::string(BOOST_BEAST_VERSION_STRING)
                                                + " websocket-server-async"
                                        );
                                    }
                                ));
                                auto* ws_ptr = ws.get();
                                ws_ptr->async_accept(
                                    request,
                                    [it,
                                     id = std::move(id),
                                     ws = std::move(ws)](boost::beast::error_code ec)
                                    {
                                        if (ec)
                                        {
                                            return;
                                        }
                                        auto connection = std::make_unique<ws::Connection>(
                                            id,
                                            8192,
                                            std::move(*ws),
                                            *it->second.impl
                                        );
                                        it->second.impl->connections.emplace(
                                            id,
                                            std::move(connection)
                                        );
                                    }
                                );
                            }
                            return;
                        }
                    }
                    {
                        const auto it = routes.find(request.target());
                        if (it != routes.end())
                        {
                            const auto& route = it->second;
                            response.set(
                                boost::beast::http::field::content_type,
                                it->second.content_type
                            );
                            auto os = boost::beast::ostream(response.body());
                            route.body->call(os);
                        }
                        else
                        {
                            response.result(boost::beast::http::status::unknown);
                            response.set(boost::beast::http::field::content_type, "text/plain");
                            boost::beast::ostream(response.body()) << "invalid\r\n";
                        }
                    }
                    break;
                }

                default:
                {
                    response.result(boost::beast::http::status::bad_request);
                    response.set(boost::beast::http::field::content_type, "text/plain");
                    break;
                }
            }

            write_response();
        }

        void write_response()
        {
            response.content_length(response.body().size());

            boost::beast::http::async_write(
                socket,
                response,
                [self = shared_from_this()](boost::beast::error_code ec, std::size_t)
                {
                    self->socket.shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);
                    self->deadline.cancel();
                }
            );
        }
    };

    struct Server::State: std::unordered_map<std::string, Route>
    {
        std::unordered_map<std::string, WebSocket> wss;
        std::unordered_map<std::string, Route> routes;
    };

    struct Server::Impl
    {
    public:
        explicit Impl(Server& parent)
            : wss(parent.state->wss)
            , routes(parent.state->routes)
            , buffer_size(parent.options.buffer)
            , acceptor(
                  context,
                  {boost::beast::net::ip::make_address("0.0.0.0"), parent.options.port}
              )
        {
            accept();
        }

        void run()
        {
            context.run();
        }

        void stop()
        {
            context.stop();
        }

    private:
        void accept()
        {
            acceptor.async_accept(
                [this](boost::beast::error_code ec, boost::asio::ip::tcp::socket socket)
                {
                    if (!ec)
                    {
                        std::make_shared<Connection>(wss, routes, buffer_size, std::move(socket))
                            ->start();
                    }
                    accept();
                }
            );
        }

        std::unordered_map<std::string, WebSocket>& wss;
        const std::unordered_map<std::string, Route>& routes;
        std::uint64_t buffer_size;

    public:
        // TODO: fix later
        boost::asio::io_context context; // NOLINT

    private:
        boost::asio::ip::tcp::acceptor acceptor;
    };

    Server::Server(Options init_options)
        : options(init_options)
        , state(std::make_unique<State>())
    {
    }

    Server::~Server() noexcept = default;

    void Server::run()
    {
        if (!impl)
        {
            auto id = "0.0.0.0:" + std::to_string(options.port);
            if (ready)
            {
                ready->call({id});
            }
            impl = std::make_unique<Impl>(*this);
            for (auto& [route, ws] : state->wss)
            {
                ws.impl->context = &impl->context;
                ws.impl->ready({id + route});
            }
        }
        impl->run();
    }

    void Server::stop()
    {
        if (impl)
        {
            impl->stop();
        }
    }

    void Server::restart()
    {
        for (auto& ws : state->wss)
        {
            ws.second.impl->context = nullptr;
        }
        impl.reset();
    }

    void Server::use_impl(
        std::string route,
        std::string content_type,
        std::unique_ptr<detail::ICallable<std::ostream&>> body
    )
    {
        state->routes.emplace(std::move(route), Route{std::move(content_type), std::move(body)});
    }

    WebSocket& Server::use_ws(std::string route)
    {
        return state->wss.emplace(std::move(route), std::make_unique<WebSocket::Impl>())
            .first->second;
    }
}
