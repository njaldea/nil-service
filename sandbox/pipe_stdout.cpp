#include <nil/service.hpp>

#include <unistd.h>

#include <iostream>
#include <string>
#include <thread>

static_assert(NIL_SERVICE_PIPE_SUPPORTED, "pipe service is not supported on this platform");

int main()
{
    auto service = nil::service::pipe::create({
        .make_read = {},
        .make_write = []() { return ::dup(STDOUT_FILENO); },
    });

    service->on_ready([](const auto& id)
                      { std::cerr << "local        : " << to_string(id) << std::endl; });
    service->on_connect([](const auto& id)
                        { std::cerr << "connected    : " << to_string(id) << std::endl; });
    service->on_disconnect([](const auto& id)
                           { std::cerr << "disconnected : " << to_string(id) << std::endl; });

    std::thread run_loop([&]() { service->start(); });

    std::string line;
    while (std::getline(std::cin, line))
    {
        service->publish(line);
    }

    service->stop();
    run_loop.join();
    return 0;
}
