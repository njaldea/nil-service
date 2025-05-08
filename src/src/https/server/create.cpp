#include <nil/service/detail/Handlers.hpp>
#include <nil/service/https/server/create.hpp>

#include "../../structs/HTTPSService.hpp"
#include "../../utils.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>

#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/stream.hpp>

#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/core/ostream.hpp>
#include <boost/beast/http/dynamic_body.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/write.hpp>

namespace nil::service
{
    struct HTTPSTransaction
    {
        boost::beast::http::request<boost::beast::http::dynamic_body>& request;   // NOLINT
        boost::beast::http::response<boost::beast::http::dynamic_body>& response; // NOLINT
    };

    std::string get_route(const HTTPSTransaction& transaction)
    {
        return transaction.request.target();
    }

    void set_content_type(const HTTPSTransaction& transaction, std::string_view type)
    {
        transaction.response.set(boost::beast::http::field::content_type, type);
    }

    void send(const HTTPSTransaction& transaction, std::string_view body)
    {
        transaction.response.result(boost::beast::http::status::ok);
        boost::beast::ostream(transaction.response.body()) << body;
    }

    void send(const HTTPSTransaction& transaction, const std::istream& body)
    {
        transaction.response.result(boost::beast::http::status::ok);
        boost::beast::ostream(transaction.response.body()) << body.rdbuf();
    }
}

namespace nil::service::https::server
{
    struct Context
    {
        explicit Context(
            const std::string& host,
            std::uint16_t port,
            const std::filesystem::path& path
        )
            : strand(make_strand(ctx))
            , ssl_context(boost::asio::ssl::context::tlsv12_server)
            , acceptor(strand, {boost::asio::ip::make_address(host), port})
        {
            ssl_context.set_options(
                boost::asio::ssl::context::default_workarounds //
                | boost::asio::ssl::context::no_sslv2          //
                | boost::asio::ssl::context::single_dh_use
            );
            ssl_context.use_certificate_chain_file(path / "cert.pem");
            ssl_context.use_private_key_file(path / "key.pem", boost::asio::ssl::context::pem);
            ssl_context.use_tmp_dh_file(path / "dh.pem"); // optional
        }

        boost::asio::io_context ctx;
        boost::asio::strand<boost::asio::io_context::executor_type> strand;
        boost::asio::ssl::context ssl_context;
        boost::asio::ip::tcp::acceptor acceptor;
    };

    struct Impl: HTTPSService
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
            boost::asio::ip::tcp::socket socket,
            boost::asio::ssl::context& ssl_ctx
        )
            : parent(init_parent)
            , stream(std::move(socket), ssl_ctx)
            , buffer(init_buffer)
            , deadline(stream.get_executor(), std::chrono::seconds(60))
        {
        }

        void start()
        {
            auto self = shared_from_this();
            stream.async_handshake(
                boost::asio::ssl::stream_base::server,
                [self](boost::beast::error_code ec)
                {
                    if (!ec)
                    {
                        self->read_request();
                    }
                }
            );
        }

        HTTPSService& parent; // NOLINT
        boost::asio::ssl::stream<boost::asio::ip::tcp::socket> stream;

        boost::beast::flat_buffer buffer;
        boost::beast::http::request<boost::beast::http::dynamic_body> request;
        boost::beast::http::response<boost::beast::http::dynamic_body> response;

        boost::asio::steady_timer deadline;

        void handle_ws(https::server::WebSocket& websocket)
        {
            namespace bb = boost::beast;
            namespace bbws = bb::websocket;

            if (bbws::is_upgrade(request))
            {
                auto id = utils::to_id(stream.next_layer().remote_endpoint());

                auto ws_stream = std::make_unique<boost::beast::websocket::stream<
                    boost::asio::ssl::stream<boost::asio::ip::tcp::socket>>>(std::move(stream));

                ws_stream->set_option(bbws::stream_base::timeout::suggested(bb::role_type::server));
                ws_stream->set_option(bbws::stream_base::decorator(
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
                        res.set(
                            bb::http::field::access_control_allow_methods,
                            "GET, POST, OPTIONS"
                        );
                    }
                ));

                auto* raw_ws = ws_stream.get(); // needed for async_accept capture
                raw_ws->async_accept(
                    request,
                    [this, &websocket, id = std::move(id), ws = std::move(ws_stream)](
                        boost::beast::error_code ec
                    )
                    {
                        if (!ec)
                        {
                            auto connection = std::make_unique<wss::Connection>(
                                id,
                                buffer.max_size(),
                                std::move(*ws),
                                websocket
                            );
                            websocket.connections.emplace(id, std::move(connection));
                        }
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
                        HTTPSTransaction transaction = {request, response};
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

        void read_request()
        {
            auto self = shared_from_this();
            boost::beast::http::async_read(
                stream,
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
        }

        void write_response()
        {
            response.content_length(response.body().size());

            boost::beast::http::async_write(
                stream,
                response,
                [self = shared_from_this()](boost::beast::error_code ec, std::size_t)
                {
                    if (ec)
                    {
                        self->stream.async_shutdown(
                            [self](boost::beast::error_code ec_shutdown)
                            {
                                (void)ec_shutdown;
                                self->deadline.cancel();
                            }
                        );
                    }
                }
            );
        }
    };

    void Impl::start()
    {
        if (!context)
        {
            context = std::make_unique<Context>(options.host, options.port, options.cert);
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
                    std::make_shared<Transaction>(
                        *this,
                        options.buffer,
                        std::move(socket),
                        context->ssl_context
                    )
                        ->start();
                }
                accept(); // continue accepting new connections
            }
        );
    }

    S create(Options options)
    {
        constexpr auto deleter = [](HTTPSService* obj) { //
            auto ptr = static_cast<Impl*>(obj);          // NOLINT
            std::default_delete<Impl>()(ptr);
        };
        return {{new Impl(std::move(options)), deleter}};
    }
}
