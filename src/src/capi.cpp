#include <nil/service/capi.h>

#include <nil/service/gateway/create.hpp>
#include <nil/service/self/create.hpp>
#include <nil/service/tcp/client/create.hpp>
#include <nil/service/tcp/server/create.hpp>
#include <nil/service/udp/client/create.hpp>
#include <nil/service/udp/server/create.hpp>
#include <nil/service/ws/client/create.hpp>
#include <nil/service/ws/server/create.hpp>

#include <chrono>
#include <memory>
#include <string>
#include <vector>

namespace
{
    struct Wrapper
    {
        std::unique_ptr<nil::service::IStandaloneService> service;
        nil::service::IGatewayService* gateway = nullptr;
    };

    Wrapper* to_wrapper(NilServiceHandle handle)
    {
        return static_cast<Wrapper*>(handle);
    }

    nil::service::IStandaloneService& to_service(NilServiceHandle handle)
    {
        return *to_wrapper(handle)->service;
    }

    NilServiceHandle make_handle(std::unique_ptr<nil::service::IStandaloneService> service)
    {
        return static_cast<NilServiceHandle>(new Wrapper{std::move(service), nullptr});
    }

    NilServiceHandle make_gateway_handle(std::unique_ptr<nil::service::IGatewayService> service)
    {
        auto* gw = service.get();
        return static_cast<NilServiceHandle>(new Wrapper{std::move(service), gw});
    }
}

extern "C"
{
    NilServiceHandle nil_service_tcp_server_create(const NilServiceTcpOptions* options)
    {
        nil::service::tcp::server::Options opts;
        if (options->host != nullptr)
        {
            opts.host = options->host;
        }
        opts.port = options->port;
        if (options->buffer > 0)
        {
            opts.buffer = options->buffer;
        }
        return make_handle(nil::service::tcp::server::create(std::move(opts)));
    }

    NilServiceHandle nil_service_tcp_client_create(const NilServiceTcpOptions* options)
    {
        nil::service::tcp::client::Options opts;
        if (options->host != nullptr)
        {
            opts.host = options->host;
        }
        opts.port = options->port;
        if (options->buffer > 0)
        {
            opts.buffer = options->buffer;
        }
        return make_handle(nil::service::tcp::client::create(std::move(opts)));
    }

    NilServiceHandle nil_service_udp_server_create(const NilServiceUdpOptions* options)
    {
        nil::service::udp::server::Options opts;
        if (options->host != nullptr)
        {
            opts.host = options->host;
        }
        opts.port = options->port;
        if (options->buffer > 0)
        {
            opts.buffer = options->buffer;
        }
        if (options->timeout_ns > 0)
        {
            opts.timeout = std::chrono::nanoseconds(options->timeout_ns);
        }
        return make_handle(nil::service::udp::server::create(std::move(opts)));
    }

    NilServiceHandle nil_service_udp_client_create(const NilServiceUdpOptions* options)
    {
        nil::service::udp::client::Options opts;
        if (options->host != nullptr)
        {
            opts.host = options->host;
        }
        opts.port = options->port;
        if (options->buffer > 0)
        {
            opts.buffer = options->buffer;
        }
        if (options->timeout_ns > 0)
        {
            opts.timeout = std::chrono::nanoseconds(options->timeout_ns);
        }
        return make_handle(nil::service::udp::client::create(std::move(opts)));
    }

    NilServiceHandle nil_service_ws_server_create(const NilServiceWsOptions* options)
    {
        nil::service::ws::server::Options opts;
        if (options->host != nullptr)
        {
            opts.host = options->host;
        }
        opts.port = options->port;
        if (options->route != nullptr && options->route[0] != '\0')
        {
            opts.route = options->route;
        }
        if (options->buffer > 0)
        {
            opts.buffer = options->buffer;
        }
        return make_handle(nil::service::ws::server::create(std::move(opts)));
    }

    NilServiceHandle nil_service_ws_client_create(const NilServiceWsOptions* options)
    {
        nil::service::ws::client::Options opts;
        if (options->host != nullptr)
        {
            opts.host = options->host;
        }
        opts.port = options->port;
        if (options->route != nullptr && options->route[0] != '\0')
        {
            opts.route = options->route;
        }
        if (options->buffer > 0)
        {
            opts.buffer = options->buffer;
        }
        return make_handle(nil::service::ws::client::create(std::move(opts)));
    }

    NilServiceHandle nil_service_self_create(void)
    {
        return make_handle(nil::service::self::create());
    }

    NilServiceHandle nil_service_gateway_create(void)
    {
        return make_gateway_handle(nil::service::gateway::create());
    }

    void nil_service_free(NilServiceHandle handle)
    {
        delete to_wrapper(handle);
    }

    void nil_service_start(NilServiceHandle handle)
    {
        to_service(handle).start();
    }

    void nil_service_stop(NilServiceHandle handle)
    {
        to_service(handle).stop();
    }

    void nil_service_restart(NilServiceHandle handle)
    {
        to_service(handle).restart();
    }

    void nil_service_dispatch(NilServiceHandle handle, NilServiceTask task, void* user_data)
    {
        to_service(handle).dispatch([task, user_data]() { task(user_data); });
    }

    void nil_service_on_ready(
        NilServiceHandle handle,
        NilServiceReadyHandler handler,
        void* user_data
    )
    {
        // c_id.text is valid for the duration of the synchronous handler() call
        // because id is alive for the entire lambda invocation.
        to_service(handle).on_ready(
            [handler, user_data](const nil::service::ID& id)
            {
                NilServiceID c_id{id.text.c_str()};
                handler(&c_id, user_data);
            }
        );
    }

    void nil_service_on_connect(
        NilServiceHandle handle,
        NilServiceConnectHandler handler,
        void* user_data
    )
    {
        // c_id.text is valid for the duration of the synchronous handler() call
        // because id is alive for the entire lambda invocation.
        to_service(handle).on_connect(
            [handler, user_data](const nil::service::ID& id)
            {
                NilServiceID c_id{id.text.c_str()};
                handler(&c_id, user_data);
            }
        );
    }

    void nil_service_on_disconnect(
        NilServiceHandle handle,
        NilServiceDisconnectHandler handler,
        void* user_data
    )
    {
        // c_id.text is valid for the duration of the synchronous handler() call
        // because id is alive for the entire lambda invocation.
        to_service(handle).on_disconnect(
            [handler, user_data](const nil::service::ID& id)
            {
                NilServiceID c_id{id.text.c_str()};
                handler(&c_id, user_data);
            }
        );
    }

    void nil_service_on_message(
        NilServiceHandle handle,
        NilServiceMessageHandler handler,
        void* user_data
    )
    {
        // c_id.text is valid for the duration of the synchronous handler() call
        // because id is alive for the entire lambda invocation.
        to_service(handle).on_message(
            [handler, user_data](const nil::service::ID& id, const void* data, std::uint64_t size)
            {
                NilServiceID c_id{id.text.c_str()};
                handler(&c_id, data, size, user_data);
            }
        );
    }

    void nil_service_publish(NilServiceHandle handle, const void* data, uint64_t size)
    {
        to_service(handle).publish(data, size);
    }

    void nil_service_publish_ex(
        NilServiceHandle handle,
        const char* const* id_texts,
        uint64_t id_count,
        const void* data,
        uint64_t size
    )
    {
        std::vector<nil::service::ID> ids;
        ids.reserve(id_count);
        for (uint64_t i = 0; i < id_count; ++i)
        {
            ids.push_back(nil::service::ID{id_texts[i]});
        }
        const auto* ptr = static_cast<const std::uint8_t*>(data);
        to_service(handle).publish_ex(std::move(ids), std::vector<std::uint8_t>(ptr, ptr + size));
    }

    void nil_service_send(
        NilServiceHandle handle,
        const char* id_text,
        const void* data,
        uint64_t size
    )
    {
        to_service(handle).send(nil::service::ID{id_text}, data, size);
    }

    void nil_service_gateway_add(NilServiceHandle gateway, NilServiceHandle service)
    {
        auto* gw_wrapper = to_wrapper(gateway);
        if (gw_wrapper->gateway != nullptr)
        {
            gw_wrapper->gateway->add_service(*to_wrapper(service)->service);
        }
    }
}
