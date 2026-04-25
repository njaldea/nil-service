#include <nil/service/pipe/create.hpp>

#include "../utils.hpp"

#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/write.hpp>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <optional>
#include <string>

namespace nil::service::pipe
{
    namespace
    {
        constexpr auto NO_FD = -1;
        constexpr auto RETRY_FD = -2;
        constexpr auto RETRY_INTERVAL_MS = 25;
    }

    struct Context
    {
        Context()
            : strand(make_strand(ctx))
            , retry_read(strand)
            , probe(strand)
        {
        }

        void reset_reader(int fd = NO_FD)
        {
            reader.reset();

            if (fd != NO_FD)
            {
                reader.emplace(strand, fd);
            }
        }

        void reset_writer(int fd = NO_FD)
        {
            writer.reset();

            if (fd != NO_FD)
            {
                writer.emplace(strand, fd);
            }
        }

        boost::asio::io_context ctx;
        boost::asio::strand<boost::asio::io_context::executor_type> strand;
        boost::asio::steady_timer retry_read;
        boost::asio::steady_timer probe;
        std::optional<boost::asio::posix::stream_descriptor> reader;
        std::optional<boost::asio::posix::stream_descriptor> writer;
    };

    struct Impl final: IStandaloneService
    {
        static std::string to_string(const void* ptr)
        {
            return static_cast<const Impl*>(ptr)->id_string();
        }

    public:
        explicit Impl(Options init_options)
            : options(std::move(init_options))
            , context(std::make_unique<Context>())
        {
            reconnect_read(true);
            reconnect_write();
        }

        ~Impl() override = default;

        Impl(Impl&&) noexcept = delete;
        Impl& operator=(Impl&&) noexcept = delete;
        Impl(const Impl&) = delete;
        Impl& operator=(const Impl&) = delete;

        void run() override
        {
            auto _ = boost::asio::make_work_guard(context->ctx);
            context->ctx.run();
        }

        void poll() override
        {
            context->ctx.poll();
        }

        void stop() override
        {
            cancel_timers();
            context->ctx.stop();
        }

        void restart() override
        {
            cancel_timers();

            read_fd = NO_FD;
            write_fd = NO_FD;
            ready_notified = false;
            connected = false;
            read_loop_active = false;
            context = std::make_unique<Context>();
            reconnect_read(false);
            reconnect_write();
        }

        void dispatch(std::function<void()> task) override
        {
            boost::asio::post(context->ctx, std::move(task));
        }

        void publish(std::vector<std::uint8_t> data) override
        {
            if (!context->writer)
            {
                return;
            }

            boost::asio::post(
                context->strand,
                [this, msg = std::move(data)]() { write_message(msg.data(), msg.size()); }
            );
        }

        void publish_ex(std::vector<ID> ids, std::vector<std::uint8_t> data) override
        {
            if (!context->writer)
            {
                return;
            }

            boost::asio::post(
                context->strand,
                [this, ids = std::move(ids), msg = std::move(data)]()
                {
                    if (!contains_self_id(ids))
                    {
                        write_message(msg.data(), msg.size());
                    }
                }
            );
        }

        void send(std::vector<ID> ids, std::vector<std::uint8_t> data) override
        {
            if (!context->writer)
            {
                return;
            }

            boost::asio::post(
                context->strand,
                [this, ids = std::move(ids), msg = std::move(data)]()
                {
                    if (contains_self_id(ids))
                    {
                        write_message(msg.data(), msg.size());
                    }
                }
            );
        }

    private:
        Options options;
        std::unique_ptr<Context> context;
        int read_fd = NO_FD;
        int write_fd = NO_FD;
        std::vector<std::uint8_t> r_buffer;
        bool ready_notified = false;
        bool connected = false;
        bool read_loop_active = false;

        std::vector<std::function<void(ID, const void*, std::uint64_t)>> on_message_cb;
        std::vector<std::function<void(ID)>> on_ready_cb;
        std::vector<std::function<void(ID)>> on_connect_cb;
        std::vector<std::function<void(ID)>> on_disconnect_cb;

        [[nodiscard]] ID self_id() const
        {
            return ID{this, this, &to_string};
        }

        [[nodiscard]] bool contains_self_id(const std::vector<ID>& ids) const
        {
            return ids.end() != std::find(ids.begin(), ids.end(), self_id());
        }

        [[nodiscard]] std::string id_string() const
        {
            if (read_fd == NO_FD)
            {
                return "pipe:read:none";
            }

            struct stat info
            {
            };

            if (::fstat(read_fd, &info) != 0)
            {
                return "pipe:read:fd=" + std::to_string(read_fd);
            }

            return "pipe:read:dev=" + std::to_string(static_cast<unsigned long long>(info.st_dev))
                + ":ino=" + std::to_string(static_cast<unsigned long long>(info.st_ino))
                + ":mode=" + std::to_string(static_cast<unsigned int>(info.st_mode));
        }

        void cancel_timers()
        {
            context->retry_read.cancel();
            context->probe.cancel();
        }

        void schedule_read_retry()
        {
            context->retry_read.expires_after(std::chrono::milliseconds(RETRY_INTERVAL_MS));
            context->retry_read.async_wait(
                [this](const boost::system::error_code& ec)
                {
                    if (!ec)
                    {
                        reconnect_read(false);
                    }
                }
            );
        }

        template <typename ResumeFn>
        void schedule_read_wait(ResumeFn resume)
        {
            if (!context->reader)
            {
                return;
            }

            context->retry_read.expires_after(std::chrono::milliseconds(RETRY_INTERVAL_MS));
            context->retry_read.async_wait(
                [this, resume = std::move(resume)](const boost::system::error_code& ec)
                {
                    if (!ec && context->reader)
                    {
                        resume();
                    }
                }
            );
        }

        void schedule_probe()
        {
            context->probe.cancel();
            if (!can_probe())
            {
                return;
            }

            context->probe.expires_after(std::chrono::milliseconds(utils::PROBE_INTERVAL_MS));
            context->probe.async_wait(
                [this](const boost::system::error_code& ec)
                {
                    if (ec || !can_probe())
                    {
                        return;
                    }

                    write_message(nullptr, 0);
                    schedule_probe();
                }
            );
        }

        void notify_ready(bool should_notify)
        {
            if (should_notify && !ready_notified)
            {
                utils::invoke(on_ready_cb, self_id());
                ready_notified = true;
            }

            if (context->reader && !read_loop_active)
            {
                read_loop_active = true;
                r_buffer.resize(options.buffer + utils::TCP_HEADER_SIZE);
                read_next_header();
            }
        }

        void notify_disconnected()
        {
            if (connected)
            {
                connected = false;
                utils::invoke(on_disconnect_cb, self_id());
            }
        }

        [[nodiscard]] bool can_probe() const
        {
            return context->reader && context->writer;
        }

        [[nodiscard]] static bool should_retry_read_later(const boost::system::error_code& ec)
        {
            return ec == boost::asio::error::would_block || ec == boost::asio::error::try_again
                || ec == boost::asio::error::interrupted;
        }

        template <typename ResumeFn>
        bool handle_read_error(const boost::system::error_code& ec, ResumeFn resume)
        {
            if (!ec)
            {
                return false;
            }

            if (ec == boost::asio::error::operation_aborted)
            {
                return true;
            }

            if (should_retry_read_later(ec))
            {
                schedule_read_wait(std::move(resume));
                return true;
            }

            handle_read_disconnect();
            return true;
        }

        bool handle_empty_read(std::size_t count)
        {
            if (count != 0)
            {
                return false;
            }

            handle_read_disconnect();
            return true;
        }

        void read_next_header()
        {
            readHeader(utils::START_INDEX, utils::TCP_HEADER_SIZE);
        }

        bool reconnect_read(bool should_notify_ready)
        {
            cancel_timers();
            read_loop_active = false;
            context->reset_reader();
            read_fd = NO_FD;

            auto read_retry = false;
            if (options.make_read)
            {
                const auto new_read_fd = options.make_read();
                if (new_read_fd >= 0)
                {
                    this->read_fd = new_read_fd;
                    context->reset_reader(this->read_fd);
                }
                else if (new_read_fd == RETRY_FD)
                {
                    read_retry = true;
                }
            }

            if (context->reader)
            {
                notify_ready(should_notify_ready);
                schedule_probe();
                return true;
            }

            if (read_retry)
            {
                schedule_read_retry();
                return true;
            }

            return !options.make_read;
        }

        bool reconnect_write()
        {
            context->probe.cancel();
            context->reset_writer();
            write_fd = NO_FD;

            if (options.make_write)
            {
                const auto new_write_fd = options.make_write();
                if (new_write_fd >= 0)
                {
                    this->write_fd = new_write_fd;
                    context->reset_writer(this->write_fd);
                }
            }

            if (context->writer)
            {
                schedule_probe();
                return true;
            }

            return !options.make_write;
        }

        void handle_read_disconnect()
        {
            cancel_timers();

            notify_disconnected();

            if (!reconnect_read(false) && !context->writer)
            {
                context->ctx.stop();
            }
        }

        bool write_message(const std::uint8_t* data, std::uint64_t size)
        {
            if (!context->writer)
            {
                return false;
            }

            const auto header = utils::to_array(size);
            boost::system::error_code ec;
            if (size == 0)
            {
                boost::asio::write(*context->writer, boost::asio::buffer(header), ec);
            }
            else
            {
                boost::asio::write(
                    *context->writer,
                    std::array<boost::asio::const_buffer, 2>{
                        boost::asio::buffer(header),
                        boost::asio::buffer(data, size)
                    },
                    ec
                );
            }

            return !ec;
        }

        void readHeader(std::uint64_t pos, std::uint64_t size)
        {
            if (!context->reader)
            {
                return;
            }

            context->reader->async_read_some(
                boost::asio::buffer(r_buffer.data() + pos, size - pos),
                [pos, size, this](const boost::system::error_code& ec, std::size_t count)
                {
                    if (handle_read_error(ec, [this, pos, size]() { readHeader(pos, size); }))
                    {
                        return;
                    }

                    if (handle_empty_read(count))
                    {
                        return;
                    }

                    if (pos + count != size)
                    {
                        readHeader(pos + count, size);
                        return;
                    }

                    if (!connected)
                    {
                        utils::invoke(on_connect_cb, self_id());
                        connected = true;
                        // Don't cancel probe - keep sending until both sides see connection
                    }

                    const auto payload_size = utils::from_array<std::uint64_t>(r_buffer.data());
                    if (payload_size == 0)
                    {
                        read_next_header();
                        return;
                    }

                    readBody(utils::START_INDEX, payload_size);
                }
            );
        }

        void readBody(std::uint64_t pos, std::uint64_t size)
        {
            if (!context->reader)
            {
                return;
            }

            if (size > options.buffer)
            {
                handle_read_disconnect();
                return;
            }

            context->reader->async_read_some(
                boost::asio::buffer(r_buffer.data() + pos, size - pos),
                [pos, size, this](const boost::system::error_code& ec, std::size_t count)
                {
                    if (handle_read_error(ec, [this, pos, size]() { readBody(pos, size); }))
                    {
                        return;
                    }

                    if (handle_empty_read(count))
                    {
                        return;
                    }

                    if (pos + count != size)
                    {
                        readBody(pos + count, size);
                    }
                    else
                    {
                        utils::invoke(on_message_cb, self_id(), r_buffer.data(), size);
                        read_next_header();
                    }
                }
            );
        }

        void impl_on_message(std::function<void(ID, const void*, std::uint64_t)> handler) override
        {
            on_message_cb.push_back(std::move(handler));
        }

        void impl_on_ready(std::function<void(ID)> handler) override
        {
            on_ready_cb.push_back(std::move(handler));
        }

        void impl_on_connect(std::function<void(ID)> handler) override
        {
            on_connect_cb.push_back(std::move(handler));
        }

        void impl_on_disconnect(std::function<void(ID)> handler) override
        {
            on_disconnect_cb.push_back(std::move(handler));
        }
    };

    std::unique_ptr<IStandaloneService> create(Options options)
    {
        return std::make_unique<Impl>(std::move(options));
    }
}
