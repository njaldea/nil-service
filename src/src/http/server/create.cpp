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
        explicit Context(std::uint16_t port)
            : strand(make_strand(ctx))
            , acceptor(strand, {boost::asio::ip::make_address("0.0.0.0"), port})
        {
        }

        boost::asio::io_context ctx;
        boost::asio::strand<boost::asio::io_context::executor_type> strand;
        boost::asio::ip::tcp::acceptor acceptor;
    };

    struct Impl: HTTPService
    {
    public:
        explicit Impl(Options init_options)
            : options(init_options)
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
        explicit Transaction(Impl& init_parent, boost::asio::ip::tcp::socket init_socket)
            : parent(init_parent)
            , socket(std::move(init_socket))
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

        HTTPService& parent; // NOLINT
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
                        auto it = parent.wss.find(request.target());
                        if (it != parent.wss.end())
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
                                    [this, it, id = std::move(id), ws = std::move(ws)](
                                        boost::beast::error_code ec
                                    )
                                    {
                                        if (!ec)
                                        {
                                            it->second.connections.emplace(
                                                id,
                                                std::make_unique<ws::Connection>(
                                                    id,
                                                    buffer.capacity(),
                                                    std::move(*ws),
                                                    it->second
                                                )
                                            );
                                        }
                                    }
                                );
                            }
                            return;
                        }
                    }
                    {
                        const auto it = parent.routes.find(request.target());
                        if (it != parent.routes.end())
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

    void Impl::start()
    {
        if (!context)
        {
            context = std::make_unique<Context>(options.port);
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
                    std::make_shared<Transaction>(*this, std::move(socket))->start();
                }
                accept();
            }
        );
    }

    H create(Options options)
    {
        constexpr auto deleter = [](HTTPService* obj) { //
            auto ptr = static_cast<Impl*>(obj);         // NOLINT
            std::default_delete<Impl>()(ptr);
        };
        return {{new Impl(options), deleter}};
    }
}
