#pragma once

#include "../ConnectedImpl.hpp"

#include <nil/service/ID.hpp>

#include <boost/asio/ip/tcp.hpp>

#include <vector>

namespace nil::service::tcp
{
    class Connection final
    {
    public:
        Connection(
            std::uint64_t buffer,
            boost::asio::ip::tcp::socket socket,
            ConnectedImpl<Connection>& impl
        );
        ~Connection() noexcept;

        Connection(Connection&&) noexcept = delete;
        Connection(const Connection&) = delete;
        Connection& operator=(Connection&&) noexcept = delete;
        Connection& operator=(const Connection&) = delete;

        void run();
        void write(const std::uint8_t* data, std::uint64_t size);
        ID remote_id() const;

        static std::string to_string_local(const void* c);
        static std::string to_string_remote(const void* c);

    private:
        void readHeader(std::uint64_t pos, std::uint64_t size);
        void readBody(std::uint64_t pos, std::uint64_t size);

        boost::asio::ip::tcp::socket socket;
        boost::asio::ip::tcp::endpoint local_endpoint;
        boost::asio::ip::tcp::endpoint remote_endpoint;
        ConnectedImpl<Connection>& impl;
        std::vector<std::uint8_t> r_buffer;
    };
}
