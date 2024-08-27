#include <nil/service/http/Server.hpp>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/core/ostream.hpp>
#include <boost/beast/http/dynamic_body.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/write.hpp>

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
            const std::unordered_map<std::string, Route>& init_routes,
            std::uint64_t buffer_size,
            boost::asio::ip::tcp::socket init_socket
        )
            : routes(init_routes)
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
                    response.result(boost::beast::http::status::ok);
                    response.set(boost::beast::http::field::server, "Beast");
                    create_response();
                    break;

                default:
                    response.result(boost::beast::http::status::bad_request);
                    response.set(boost::beast::http::field::content_type, "text/plain");
                    break;
            }

            write_response();
        }

        void create_response()
        {
            const auto it = routes.find(request.target());
            if (it != routes.end())
            {
                const auto& route = it->second;
                response.set(boost::beast::http::field::content_type, it->second.content_type);
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

    struct Server::Routes: std::unordered_map<std::string, Route>
    {
    };

    struct Server::Impl
    {
    public:
        explicit Impl(Server& parent)
            : routes(*parent.routes)
            , buffer_size(parent.options.buffer)
            , context(1)
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
                        // TODO: not thread safe when user tries to stop the service.
                        //  routes might die before the main thread finishes.
                        //  users are expected to wait for the parent thread to join
                        //  before destroying the service.
                        std::make_shared<Connection>(routes, buffer_size, std::move(socket))
                            ->start();
                    }
                    accept();
                }
            );
        }

        const std::unordered_map<std::string, Route>& routes;
        std::uint64_t buffer_size;

        boost::asio::io_context context;
        boost::asio::ip::tcp::acceptor acceptor;
    };

    Server::Server(Options init_options)
        : options(init_options)
        , routes(std::make_unique<Routes>())
        , impl(std::make_unique<Impl>(*this))
    {
    }

    Server::~Server() noexcept = default;

    void Server::run()
    {
        if (ready)
        {
            ready->call({"0.0.0.0:" + std::to_string(options.port)});
        }
        impl->run();
    }

    void Server::stop()
    {
        impl->stop();
    }

    void Server::restart()
    {
        impl.reset();
        impl = std::make_unique<Impl>(*this);
    }

    void Server::use_impl(
        std::string route,
        std::string content_type,
        std::unique_ptr<detail::ICallable<std::ostream&>> body
    )
    {
        routes->emplace(std::move(route), Route{std::move(content_type), std::move(body)});
    }
}
