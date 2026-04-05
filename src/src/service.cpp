#include <nil/service.h>

#include <nil/service.hpp>
#include <nil/xalt/raii.hpp>

#include <chrono>
#include <memory>
#include <string_view>
#include <vector>

namespace
{
    static_assert(sizeof(nil_service_id::_) == sizeof(nil::service::ID::to_string));

    nil_service_id from_cpp_id(nil::service::ID id)
    {
        return {
            .owner = id.owner,
            .id = id.id,
            ._ = reinterpret_cast<void*>(id.to_string) // NOLINT
        };
    }

    nil::service::ID to_cpp_id(const nil_service_id* id)
    {
        using T = std::string (*)(const void*);
        return {id->owner, id->id, reinterpret_cast<T>(id->_)}; // NOLINT
    }

    std::vector<nil::service::ID> to_cpp_ids(const nil_service_ids* ids)
    {
        std::vector<nil::service::ID> retval;
        if (ids == nullptr || ids->ids == nullptr)
        {
            return retval;
        }

        for (auto i = 0U; i < ids->size; ++i)
        {
            const auto id = to_cpp_id(&ids->ids[i]);
            retval.emplace_back(id);
        }

        return retval;
    }

    std::vector<std::uint8_t> to_payload(const void* data, std::uint64_t size)
    {
        if (data == nullptr || size == 0U)
        {
            return {};
        }

        const auto* begin = static_cast<const std::uint8_t*>(data);
        return {begin, begin + size};
    }
}

extern "C"
{
    nil_service_gateway nil_service_create_gateway(void)
    {
        auto* gateway = nil::service::gateway::create().release();
        return {.handle = gateway};
    }

    void nil_service_gateway_add_service(nil_service_gateway gateway, nil_service_event service)
    {
        auto* event = static_cast<nil::service::IEventService*>(service.handle);
        static_cast<nil::service::IGatewayService*>(gateway.handle)->add_service(*event);
    }

    void nil_service_gateway_destroy(nil_service_gateway gateway)
    {
        delete static_cast<nil::service::IGatewayService*>(gateway.handle); // NOLINT
    }

    void nil_service_runnable_start(nil_service_runnable service)
    {
        static_cast<nil::service::IRunnableService*>(service.handle)->start();
    }

    void nil_service_runnable_stop(nil_service_runnable service)
    {
        static_cast<nil::service::IRunnableService*>(service.handle)->stop();
    }

    void nil_service_runnable_restart(nil_service_runnable service)
    {
        static_cast<nil::service::IRunnableService*>(service.handle)->restart();
    }

    void nil_service_runnable_dispatch(
        nil_service_runnable service,
        nil_service_dispatch_info callback
    )
    {
        auto holder = std::make_shared<nil::xalt::raii<void>>(callback.context, callback.cleanup);
        static_cast<nil::service::IRunnableService*>(service.handle)
            ->dispatch([exec = callback.exec, holder]() { exec(holder->object); });
    }

    void nil_service_id_print(nil_service_id id)
    {
        printf("%s\n", to_string(to_cpp_id(&id)).c_str());
    }

    void nil_service_message_publish(nil_service_message service, const void* data, uint64_t size)
    {
        static_cast<nil::service::IMessageService*>(service.handle)
            ->publish(to_payload(data, size));
    }

    void nil_service_message_publish_ex(
        nil_service_id id,
        nil_service_message service,
        const void* data,
        uint64_t size
    )
    {
        static_cast<nil::service::IMessageService*>(service.handle)
            ->publish_ex({to_cpp_id(&id)}, to_payload(data, size));
    }

    void nil_service_message_send(
        nil_service_id id,
        nil_service_message service,
        const void* data,
        uint64_t size
    )
    {
        static_cast<nil::service::IMessageService*>(service.handle)
            ->send({to_cpp_id(&id)}, to_payload(data, size));
    }

    void nil_service_message_publish_ex_ids(
        nil_service_ids ids,
        nil_service_message service,
        const void* data,
        uint64_t size
    )
    {
        static_cast<nil::service::IMessageService*>(service.handle)
            ->publish_ex(to_cpp_ids(&ids), to_payload(data, size));
    }

    void nil_service_message_send_ids(
        nil_service_ids ids,
        nil_service_message service,
        const void* data,
        uint64_t size
    )
    {
        static_cast<nil::service::IMessageService*>(service.handle)
            ->send(to_cpp_ids(&ids), to_payload(data, size));
    }

    void nil_service_callback_on_ready(
        nil_service_callback service,
        nil_service_callback_info callback
    )
    {
        auto holder = std::make_shared<nil::xalt::raii<void>>(callback.context, callback.cleanup);
        static_cast<nil::service::ICallbackService*>(service.handle)
            ->on_ready(
                [exec = callback.exec, holder](const nil::service::ID& id)
                {
                    (void)id;
                    (void)exec;
                    auto c_id = from_cpp_id(id);
                    exec(&c_id, holder->object);
                }
            );
    }

    void nil_service_callback_on_connect(
        nil_service_callback service,
        nil_service_callback_info callback
    )
    {
        auto holder = std::make_shared<nil::xalt::raii<void>>(callback.context, callback.cleanup);
        static_cast<nil::service::ICallbackService*>(service.handle)
            ->on_connect(
                [exec = callback.exec, holder](const nil::service::ID& id)
                {
                    auto c_id = from_cpp_id(id);
                    exec(&c_id, holder->object);
                }
            );
    }

    void nil_service_callback_on_disconnect(
        nil_service_callback service,
        nil_service_callback_info callback
    )
    {
        auto holder = std::make_shared<nil::xalt::raii<void>>(callback.context, callback.cleanup);
        static_cast<nil::service::ICallbackService*>(service.handle)
            ->on_disconnect(
                [exec = callback.exec, holder](const nil::service::ID& id)
                {
                    auto c_id = from_cpp_id(id);
                    exec(&c_id, holder->object);
                }
            );
    }

    void nil_service_callback_on_message(
        nil_service_callback service,
        nil_service_msg_callback_info callback
    )
    {
        auto holder = std::make_shared<nil::xalt::raii<void>>(callback.context, callback.cleanup);
        static_cast<nil::service::ICallbackService*>(service.handle)
            ->on_message(
                [exec = callback.exec,
                 holder](const nil::service::ID& id, const void* data, std::uint64_t size)
                {
                    auto c_id = from_cpp_id(id);
                    exec(&c_id, data, size, holder->object);
                }
            );
    }

    void nil_service_web_on_ready(nil_service_web service, nil_service_callback_info callback)
    {
        auto holder = std::make_shared<nil::xalt::raii<void>>(callback.context, callback.cleanup);
        static_cast<nil::service::IWebService*>(service.handle)
            ->on_ready(
                [exec = callback.exec, holder](const nil::service::ID& id)
                {
                    auto c_id = from_cpp_id(id);
                    exec(&c_id, holder->object);
                }
            );
    }

    void nil_service_web_on_get(nil_service_web service, nil_service_web_get_callback_info callback)
    {
        auto holder = std::make_shared<nil::xalt::raii<void>>(callback.context, callback.cleanup);
        static_cast<nil::service::IWebService*>(service.handle)
            ->on_get(
                [exec = callback.exec, holder](nil::service::WebTransaction& transaction)
                {
                    if (exec == nullptr)
                    {
                        return false;
                    }
                    const auto c_transaction = nil_service_web_transaction{.handle = &transaction};
                    return exec(c_transaction, holder->object) != 0;
                }
            );
    }

    nil_service_event nil_service_web_use_ws(nil_service_web service, const char* route)
    {
        auto* event = static_cast<nil::service::IWebService*>(service.handle)
                          ->use_ws(route == nullptr ? "/" : route);
        return {.handle = event};
    }

    void nil_service_web_transaction_set_content_type(
        nil_service_web_transaction transaction,
        const char* content_type
    )
    {
        if (content_type == nullptr)
        {
            return;
        }
        auto& tx = *static_cast<nil::service::WebTransaction*>(transaction.handle);
        nil::service::set_content_type(tx, content_type);
    }

    void nil_service_web_transaction_send(
        nil_service_web_transaction transaction,
        const void* body,
        uint64_t size
    )
    {
        const auto* ptr = static_cast<const char*>(body);
        auto& tx = *static_cast<nil::service::WebTransaction*>(transaction.handle);
        nil::service::send(tx, std::string_view{ptr == nullptr ? "" : ptr, size});
    }

    const char* nil_service_web_transaction_get_route(
        nil_service_web_transaction transaction,
        uint64_t* size
    )
    {
        auto& tx = *static_cast<nil::service::WebTransaction*>(transaction.handle);
        const auto route = nil::service::get_route(tx);
        if (size != nullptr)
        {
            *size = route.size();
        }
        return route.data();
    }

    nil_service_message nil_service_event_to_message(nil_service_event service)
    {
        auto* event = static_cast<nil::service::IEventService*>(service.handle);
        return {.handle = static_cast<nil::service::IMessageService*>(event)};
    }

    nil_service_callback nil_service_event_to_callback(nil_service_event service)
    {
        auto* event = static_cast<nil::service::IEventService*>(service.handle);
        return {.handle = static_cast<nil::service::ICallbackService*>(event)};
    }

    nil_service_standalone nil_service_gateway_to_standalone(nil_service_gateway service)
    {
        auto* gateway = static_cast<nil::service::IGatewayService*>(service.handle);
        return {.handle = static_cast<nil::service::IStandaloneService*>(gateway)};
    }

    nil_service_event nil_service_gateway_to_event(nil_service_gateway service)
    {
        auto* gateway = static_cast<nil::service::IGatewayService*>(service.handle);
        return {.handle = static_cast<nil::service::IEventService*>(gateway)};
    }

    nil_service_message nil_service_gateway_to_message(nil_service_gateway service)
    {
        auto* gateway = static_cast<nil::service::IGatewayService*>(service.handle);
        return {.handle = static_cast<nil::service::IMessageService*>(gateway)};
    }

    nil_service_callback nil_service_gateway_to_callback(nil_service_gateway service)
    {
        auto* gateway = static_cast<nil::service::IGatewayService*>(service.handle);
        return {.handle = static_cast<nil::service::ICallbackService*>(gateway)};
    }

    nil_service_runnable nil_service_gateway_to_runnable(nil_service_gateway service)
    {
        auto* gateway = static_cast<nil::service::IGatewayService*>(service.handle);
        return {.handle = static_cast<nil::service::IRunnableService*>(gateway)};
    }

    nil_service_message nil_service_standalone_to_message(nil_service_standalone service)
    {
        auto* standalone = static_cast<nil::service::IStandaloneService*>(service.handle);
        return {.handle = static_cast<nil::service::IMessageService*>(standalone)};
    }

    nil_service_callback nil_service_standalone_to_callback(nil_service_standalone service)
    {
        auto* standalone = static_cast<nil::service::IStandaloneService*>(service.handle);
        return {.handle = static_cast<nil::service::ICallbackService*>(standalone)};
    }

    nil_service_event nil_service_standalone_to_event(nil_service_standalone service)
    {
        auto* standalone = static_cast<nil::service::IStandaloneService*>(service.handle);
        return {.handle = static_cast<nil::service::IEventService*>(standalone)};
    }

    nil_service_runnable nil_service_standalone_to_runnable(nil_service_standalone service)
    {
        auto* standalone = static_cast<nil::service::IStandaloneService*>(service.handle);
        return {.handle = static_cast<nil::service::IRunnableService*>(standalone)};
    }

    nil_service_standalone nil_service_create_udp_client(
        const char* host,
        uint16_t port,
        uint64_t buffer,
        double timeout_s
    )
    {
        const auto options = nil::service::udp::client::Options{
            .host = host,
            .port = port,
            .buffer = buffer,
            .timeout = std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::duration<double>(timeout_s)
            )
        };
        return {.handle = create(options).release()};
    }

    nil_service_standalone nil_service_create_udp_server(
        const char* host,
        uint16_t port,
        uint64_t buffer,
        double timeout_s
    )
    {
        const auto options = nil::service::udp::server::Options{
            .host = host,
            .port = port,
            .buffer = buffer,
            .timeout = std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::duration<double>(timeout_s)
            )
        };
        return {.handle = create(options).release()};
    }

    nil_service_standalone nil_service_create_tcp_client(
        const char* host,
        uint16_t port,
        uint64_t buffer
    )
    {
        const auto options
            = nil::service::tcp::client::Options{.host = host, .port = port, .buffer = buffer};
        return {.handle = create(options).release()};
    }

    nil_service_standalone nil_service_create_tcp_server(
        const char* host,
        uint16_t port,
        uint64_t buffer
    )
    {
        const auto options
            = nil::service::tcp::server::Options{.host = host, .port = port, .buffer = buffer};
        return {.handle = create(options).release()};
    }

#if defined(__unix__) || defined(__unix) || defined(unix) || defined(__APPLE__)
    int nil_service_pipe_mkfifo(const char* path)
    {
        if (path == nullptr)
        {
            return -1;
        }
        return nil::service::pipe::mkfifo(path);
    }

    int nil_service_pipe_r_mkfifo(const char* path)
    {
        if (path == nullptr)
        {
            return -1;
        }
        return nil::service::pipe::r_mkfifo(path);
    }

    int nil_service_pipe_w_mkfifo(const char* path)
    {
        if (path == nullptr)
        {
            return -1;
        }
        return nil::service::pipe::w_mkfifo(path);
    }

    nil_service_standalone nil_service_create_pipe(
        nil_service_pipe_fd_provider read_fd,
        nil_service_pipe_fd_provider write_fd,
        uint64_t buffer
    )
    {
        auto read_holder
            = std::make_shared<nil::xalt::raii<void>>(read_fd.context, read_fd.cleanup);
        auto write_holder
            = std::make_shared<nil::xalt::raii<void>>(write_fd.context, write_fd.cleanup);

        const auto options = nil::service::pipe::Options{
            .make_read =
                [exec = read_fd.exec, holder = read_holder]()
            {
                if (exec == nullptr)
                {
                    return -1;
                }

                return exec(holder->object);
            },
            .make_write =
                [exec = write_fd.exec, holder = write_holder]()
            {
                if (exec == nullptr)
                {
                    return -1;
                }

                return exec(holder->object);
            },
            .buffer = buffer,
        };
        return {.handle = nil::service::pipe::create(options).release()};
    }
#endif

    nil_service_standalone nil_service_create_ws_client(
        const char* host,
        uint16_t port,
        const char* route,
        uint64_t buffer
    )
    {
        const auto options = nil::service::ws::client::Options{
            .host = host,
            .port = port,
            .route = route == nullptr ? "/" : route,
            .buffer = buffer
        };
        return {.handle = create(options).release()};
    }

    nil_service_standalone nil_service_create_ws_server(
        const char* host,
        uint16_t port,
        const char* route,
        uint64_t buffer
    )
    {
        const auto options = nil::service::ws::server::Options{
            .host = host,
            .port = port,
            .route = route == nullptr ? "/" : route,
            .buffer = buffer
        };
        return {.handle = create(options).release()};
    }

    nil_service_web nil_service_create_http_server(const char* host, uint16_t port, uint64_t buffer)
    {
        const auto options
            = nil::service::http::server::Options{.host = host, .port = port, .buffer = buffer};
        return {.handle = create(options).release()};
    }

    nil_service_runnable nil_service_web_to_runnable(nil_service_web service)
    {
        auto* web = static_cast<nil::service::IWebService*>(service.handle);
        return {.handle = static_cast<nil::service::IRunnableService*>(web)};
    }

    void nil_service_web_destroy(nil_service_web service)
    {
        delete static_cast<nil::service::IWebService*>(service.handle); // NOLINT
    }

    void nil_service_standalone_destroy(nil_service_standalone service)
    {
        delete static_cast<nil::service::IStandaloneService*>(service.handle); // NOLINT
    }
}
