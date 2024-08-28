#include "Connection.hpp"

namespace nil::service::ws
{
    Connection::Connection(
        ID ini_id,
        std::uint64_t init_buffer,
        boost::beast::websocket::stream<boost::beast::tcp_stream> init_ws,
        ConnectedImpl<Connection>& init_impl
    )
        : identifier(std::move(ini_id))
        , ws(std::move(init_ws))
        , flat_buffer(init_buffer)
        , impl(init_impl)
    {
        ws.binary(true);
        impl.connect(this);
        read();
    }

    Connection::~Connection() noexcept = default;

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

                impl.message(id(), flat_buffer.cdata().data(), count);
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

    const ID& Connection::id() const
    {
        return identifier;
    }
}
