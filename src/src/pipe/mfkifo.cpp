#include <nil/service/pipe/create.hpp>

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <string_view>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace nil::service::pipe
{
    namespace
    {
        constexpr auto NO_FD = -1;
        constexpr auto RETRY_FD = -2;

        bool ensure_fifo(std::string_view path)
        {
            if (::mkfifo(path.data(), 0666) != 0 && errno != EEXIST)
            {
                std::printf("mkfifo(%s) failed: %s\n", path.data(), std::strerror(errno));
                return false;
            }

            struct stat info
            {
            };

            if (::stat(path.data(), &info) != 0)
            {
                std::printf("stat(%s) failed: %s\n", path.data(), std::strerror(errno));
                return false;
            }

            if (!S_ISFIFO(info.st_mode))
            {
                std::printf("%s exists but is not a fifo\n", path.data());
                return false;
            }

            return true;
        }

        int open_fifo(std::string_view path, int flags)
        {
            if (path.empty())
            {
                return NO_FD;
            }

            if (!ensure_fifo(path))
            {
                return NO_FD;
            }

            const auto fd = ::open(path.data(), flags | O_NONBLOCK);
            if (fd >= 0)
            {
                return fd;
            }

            if (errno == ENXIO || errno == EAGAIN || errno == EWOULDBLOCK)
            {
                return RETRY_FD;
            }

            std::printf("open(%s) failed: %s\n", path.data(), std::strerror(errno));
            return NO_FD;
        }
    }

    int mkfifo(std::string_view path)
    {
        return open_fifo(path, O_RDWR);
    }

    int r_mkfifo(std::string_view path)
    {
        return open_fifo(path, O_RDONLY);
    }

    int w_mkfifo(std::string_view path)
    {
        return open_fifo(path, O_WRONLY);
    }
}
