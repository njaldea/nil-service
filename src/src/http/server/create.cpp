#include <nil/service/http/server/create.hpp>

#include "../../structs/HTTPService.hpp"
#include "../../utils.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <boost/asio/strand.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/core/ostream.hpp>
#include <boost/beast/http/dynamic_body.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/write.hpp>

namespace nil::service::http::server
{
    struct Context
    {
        explicit Context(const std::string& host, std::uint16_t port)
            : strand(make_strand(ctx))
            , acceptor(strand)
        {
            const auto endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::make_address(host), port);
            acceptor.open(endpoint.protocol());
            acceptor.set_option(boost::asio::socket_base::reuse_address(true));
            acceptor.bind(endpoint);
            acceptor.listen();
        }

        boost::asio::io_context ctx;
        boost::asio::strand<boost::asio::io_context::executor_type> strand;
        boost::asio::ip::tcp::acceptor acceptor;
    };

    struct Impl: HTTPService
    {
    public:
        explicit Impl(Options init_options)
            : options(std::move(init_options))
        {
        }

        void start() override;
        void stop() override;
        void restart() override;

    private:
        void accept();

        Options options;
        std::unique_ptr<Context> context;
    };

    struct Transaction final: public std::enable_shared_from_this<Transaction>
    {
        explicit Transaction(
            Impl& init_parent,
            std::uint64_t init_buffer,
            boost::asio::ip::tcp::socket init_socket
        )
            : parent(init_parent)
            , socket(std::move(init_socket))
            , buffer(init_buffer)
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
                [self](boost::beast::error_code ec, std::size_t /* bytes_transferred */)
                {
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

        HTTPService& parent; // NOLINT
        boost::asio::ip::tcp::socket socket;

        boost::beast::flat_buffer buffer;
        boost::beast::http::request<boost::beast::http::dynamic_body> request;
        boost::beast::http::response<boost::beast::http::dynamic_body> response;

        boost::asio::steady_timer deadline;

        void handle_ws(http::server::WebSocket& websocket)
        {
            namespace bb = boost::beast;
            namespace bbws = bb::websocket;
            if (bbws::is_upgrade(request))
            {
                auto ws = std::make_unique<bbws::stream<bb::tcp_stream>>(std::move(socket));
                ws->set_option(bbws::stream_base::timeout::suggested(bb::role_type::server));
                ws->set_option(bbws::stream_base::decorator(
                    [](bbws::response_type& res)
                    {
                        res.set(
                            bb::http::field::server,
                            BOOST_BEAST_VERSION_STRING " websocket-server-async"
                        );
                        res.set(bb::http::field::access_control_allow_origin, "*");
                        res.set(
                            bb::http::field::access_control_allow_headers,
                            "origin, x-requested-with, content-type"
                        );
                        res.set(bb::http::field::access_control_allow_methods, "GET");
                    }
                ));

                auto* ws_ptr = ws.get();
                ws_ptr->async_accept(
                    request,
                    [&websocket, s = buffer.max_size(), ws = std::move(ws)] //
                    (bb::error_code ec)
                    {
                        if (ec)
                        {
                            return;
                        }

                        auto id = utils::to_id(ws->next_layer().socket().remote_endpoint());
                        auto connection
                            = std::make_unique<ws::Connection>(id, s, std::move(*ws), websocket);
                        websocket.connections.emplace(id, std::move(connection));
                    }
                );
            }
        }

        void process_request()
        {
            response.version(request.version());
            response.keep_alive(false);

            switch (request.method())
            {
                case boost::beast::http::verb::get:
                {
                    response.set(boost::beast::http::field::server, "Beast");
                    auto it = parent.wss.find(request.target());
                    if (it != parent.wss.end())
                    {
                        response.result(boost::beast::http::status::ok);
                        handle_ws(it->second);
                    }
                    else if (parent.on_get)
                    {
                        response.result(boost::beast::http::status::bad_request);
                        WebTransaction transaction = {request, response};
                        parent.on_get(transaction);
                        write_response();
                    }
                }
                break;
                default:
                {
                    response.result(boost::beast::http::status::bad_request);
                    response.set(boost::beast::http::field::content_type, "text/plain");
                    write_response();
                    break;
                }
            }
        }

        void write_response()
        {
            response.content_length(response.body().size());

            boost::beast::http::async_write(
                socket,
                response,
                [self = shared_from_this()](boost::beast::error_code ec, std::size_t)
                {
                    if (ec)
                    {
                        return;
                    }
                    self->socket.shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);
                    self->deadline.cancel();
                }
            );
        }
    };

    void Impl::start()
    {
        if (!context)
        {
            context = std::make_unique<Context>(options.host, options.port);
            auto id = utils::to_id(context->acceptor.local_endpoint());
            detail::invoke(on_ready, id);
            for (auto& [route, ws] : wss)
            {
                ws.context = &context->ctx;
                ws.ready({id.text + route});
            }
            accept();
        }
        context->ctx.run();
    }

    void Impl::stop()
    {
        if (context)
        {
            context->ctx.stop();
        }
    }

    void Impl::restart()
    {
        context.reset();
    }

    void Impl::accept()
    {
        context->acceptor.async_accept(
            [this](boost::beast::error_code ec, boost::asio::ip::tcp::socket socket)
            {
                if (!ec)
                {
                    std::make_shared<Transaction>(*this, options.buffer, std::move(socket))
                        ->start();
                }
                accept();
            }
        );
    }

    W create(Options options)
    {
        constexpr auto deleter = [](WebService* obj) { //
            auto ptr = static_cast<Impl*>(obj);        // NOLINT
            std::default_delete<Impl>()(ptr);
        };
        return {{new Impl(std::move(options)), deleter}};
    }
}
