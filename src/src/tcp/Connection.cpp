#include "Connection.hpp"

#include "../utils.hpp"

namespace nil::service::tcp
{
    Connection::Connection(
        std::uint64_t buffer,
        boost::asio::ip::tcp::socket init_socket,
        ConnectedImpl<Connection>& init_impl
    )
        : socket(std::move(init_socket))
        , local_endpoint(socket.local_endpoint())
        , remote_endpoint(socket.remote_endpoint())
        , impl(init_impl)
    {
        r_buffer.resize(buffer);
    }

    Connection::~Connection() noexcept = default;

    void Connection::start()
    {
        readHeader(utils::START_INDEX, utils::TCP_HEADER_SIZE);
        impl.connect(this);
    }

    void Connection::readHeader(std::uint64_t pos, std::uint64_t size)
    {
        socket.async_read_some(
            boost::asio::buffer(r_buffer.data() + pos, size - pos),
            [pos, size, this](
                const boost::system::error_code& ec,
                std::size_t count //
            )
            {
                if (ec)
                {
                    impl.disconnect(this);
                    return;
                }

                if (pos + count != size)
                {
                    readHeader(pos + count, size);
                }
                else
                {
                    readBody(utils::START_INDEX, utils::from_array<std::uint64_t>(r_buffer.data()));
                }
            }
        );
    }

    void Connection::readBody(std::uint64_t pos, std::uint64_t size)
    {
        socket.async_read_some(
            boost::asio::buffer(r_buffer.data() + pos, size - pos),
            [pos, size, this](const boost::system::error_code& ec, std::size_t count)
            {
                if (ec)
                {
                    impl.disconnect(this);
                    return;
                }

                if (pos + count != size)
                {
                    readBody(pos + count, size);
                }
                else
                {
                    impl.message(remote_id(), r_buffer.data(), size);
                    readHeader(utils::START_INDEX, utils::TCP_HEADER_SIZE);
                }
            }
        );
    }

    void Connection::write(const std::uint8_t* data, std::uint64_t size)
    {
        boost::system::error_code ec;
        socket.write_some(
            std::array<boost::asio::const_buffer, 2>{
                boost::asio::buffer(utils::to_array(size)),
                boost::asio::buffer(data, size)
            },
            ec
        );
    }

    std::string Connection::to_string_local(const void* c)
    {
        return utils::to_id(static_cast<const Connection*>(c)->local_endpoint);
    }

    std::string Connection::to_string_remote(const void* c)
    {
        return utils::to_id(static_cast<const Connection*>(c)->remote_endpoint);
    }

    ID Connection::remote_id() const
    {
        return ID{&impl, this, &to_string_remote};
    }
}
