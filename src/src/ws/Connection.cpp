#include "Connection.hpp"
#include "../utils.hpp"

namespace nil::service::ws
{
    Connection::Connection(
        std::uint64_t init_buffer,
        boost::beast::websocket::stream<boost::beast::tcp_stream> init_ws,
        ConnectedImpl<Connection>& init_impl
    )
        : ws(std::move(init_ws))
        , flat_buffer(init_buffer)
        , impl(init_impl)
    {
        ws.binary(true);
    }

    Connection::~Connection() noexcept = default;

    void Connection::start()
    {
        impl.connect(this);
        read();
    }

    void Connection::read()
    {
        ws.async_read(
            flat_buffer,
            [this](boost::beast::error_code ec, std::size_t count)
            {
                if (ec)
                {
                    impl.disconnect(this);
                    return;
                }

                impl.message(remote_id(), flat_buffer.cdata().data(), count);
                flat_buffer.consume(count);
                read();
            }
        );
    }

    void Connection::write(const std::uint8_t* data, std::uint64_t size)
    {
        boost::system::error_code ec;
        ws.write(boost::asio::buffer(data, size), ec);
    }

    std::string Connection::to_string_local(const void* c)
    {
        return utils::to_id(
            static_cast<const Connection*>(c)->ws.next_layer().socket().local_endpoint()
        );
    }

    std::string Connection::to_string_remote(const void* c)
    {
        return utils::to_id(
            static_cast<const Connection*>(c)->ws.next_layer().socket().remote_endpoint()
        );
    }

    ID Connection::remote_id() const
    {
        return ID{&impl, this, &to_string_remote};
    }
}
