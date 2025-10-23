#pragma once

#include "../ConnectedImpl.hpp"

#include <nil/service/ID.hpp>

#define BOOST_ASIO_STANDALONE
#define BOOST_ASIO_NO_TYPEID
#include <boost/asio/io_context.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

namespace nil::service::ws
{
    class Connection final
    {
    public:
        Connection(
            ID ini_id,
            std::uint64_t init_buffer,
            boost::beast::websocket::stream<boost::beast::tcp_stream> init_ws,
            ConnectedImpl<Connection>& init_impl
        );
        ~Connection() noexcept;

        Connection(Connection&&) noexcept = delete;
        Connection& operator=(Connection&&) noexcept = delete;

        Connection(const Connection&) = delete;
        Connection& operator=(const Connection&) = delete;

        void write(const std::uint8_t* data, std::uint64_t size);
        const ID& id() const;

    private:
        void read();

        ID identifier;
        boost::beast::websocket::stream<boost::beast::tcp_stream> ws;
        boost::beast::flat_buffer flat_buffer;
        ConnectedImpl<Connection>& impl;
    };
}
