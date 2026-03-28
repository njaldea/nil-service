#include <nil/service.h>

#include <nil/service/structs.hpp>

#include <memory>
#include <vector>

namespace
{
    using nil::service::ICallbackService;
    using nil::service::ID;
    using nil::service::IEventService;
    using nil::service::IMessageService;
    using nil::service::IRunnableService;
    using nil::service::IStandaloneService;

    template <typename T, typename Opaque>
    T* as(Opaque* ptr)
    {
        return static_cast<T*>(static_cast<void*>(ptr));
    }

    template <typename T, typename Opaque>
    const T* as(const Opaque* ptr)
    {
        return static_cast<const T*>(static_cast<const void*>(ptr));
    }

    struct ContextCleanup
    {
        explicit ContextCleanup(void* init_context, void (*init_cleanup)(void*))
            : context(init_context)
            , cleanup(init_cleanup)
        {
        }

        ContextCleanup(const ContextCleanup&) = default;
        ContextCleanup(ContextCleanup&&) = default;
        ContextCleanup& operator=(const ContextCleanup&) = default;
        ContextCleanup& operator=(ContextCleanup&&) = default;

        ~ContextCleanup()
        {
            if (cleanup != nullptr)
            {
                cleanup(context);
            }
        }

        void* context;
        void (*cleanup)(void*);
    };

    std::vector<ID> to_cpp_ids(const nil_service_ids* ids)
    {
        if (ids == nullptr || ids->ids == nullptr || ids->size == 0U)
        {
            return {};
        }

        const auto* begin = as<const ID>(ids->ids);
        const auto* end = begin + ids->size;
        return {begin, end};
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

void nil_service_runnable_start(struct nil_service_runnable* service)
{
    as<IRunnableService>(service)->start();
}

void nil_service_runnable_stop(struct nil_service_runnable* service)
{
    as<IRunnableService>(service)->stop();
}

void nil_service_runnable_restart(struct nil_service_runnable* service)
{
    as<IRunnableService>(service)->restart();
}

void nil_service_runnable_dispatch(
    struct nil_service_runnable* service,
    struct nil_service_dispatch_info callback
)
{
    auto holder = std::make_shared<ContextCleanup>(callback.context, callback.cleanup);
    as<IRunnableService>(service)->dispatch([exec = callback.exec, holder]()
                                            { exec(holder->context); });
}

void nil_service_message_publish(
    struct nil_service_message* service,
    const void* data,
    uint64_t size
)
{
    as<IMessageService>(service)->publish(to_payload(data, size));
}

void nil_service_message_publish_ex(
    struct nil_service_id* id,
    struct nil_service_message* service,
    const char* data,
    uint64_t size
)
{
    as<IMessageService>(service)->publish_ex({*as<ID>(id)}, to_payload(data, size));
}

void nil_service_message_send(
    struct nil_service_id* id,
    struct nil_service_message* service,
    const char* data,
    uint64_t size
)
{
    as<IMessageService>(service)->send(*as<ID>(id), to_payload(data, size));
}

void nil_service_message_publish_ex_ids(
    struct nil_service_ids* ids,
    struct nil_service_message* service,
    const char* data,
    uint64_t size
)
{
    as<IMessageService>(service)->publish_ex(to_cpp_ids(ids), to_payload(data, size));
}

void nil_service_message_send_ids(
    struct nil_service_id* id,
    struct nil_service_message* service,
    const char* data,
    uint64_t size
)
{
    as<IMessageService>(service)->send(*as<ID>(id), to_payload(data, size));
}

void nil_service_callback_on_ready(
    struct nil_service_callback* service,
    struct nil_service_callback_info callback
)
{
    auto holder = std::make_shared<ContextCleanup>(callback.context, callback.cleanup);
    as<ICallbackService>(service)->on_ready(
        [exec = callback.exec, holder](const ID& id)
        {
            auto mutable_id = id;
            exec(as<nil_service_id>(&mutable_id), holder->context);
        }
    );
}

void nil_service_callback_on_connect(
    struct nil_service_callback* service,
    struct nil_service_callback_info callback
)
{
    auto holder = std::make_shared<ContextCleanup>(callback.context, callback.cleanup);
    as<ICallbackService>(service)->on_connect(
        [exec = callback.exec, holder](const ID& id)
        {
            auto mutable_id = id;
            exec(as<nil_service_id>(&mutable_id), holder->context);
        }
    );
}

void nil_service_callback_on_disconnect(
    struct nil_service_callback* service,
    struct nil_service_callback_info callback
)
{
    auto holder = std::make_shared<ContextCleanup>(callback.context, callback.cleanup);
    as<ICallbackService>(service)->on_disconnect(
        [exec = callback.exec, holder](const ID& id)
        {
            auto mutable_id = id;
            exec(as<nil_service_id>(&mutable_id), holder->context);
        }
    );
}

void nil_service_callback_on_message(
    struct nil_service_callback* service,
    struct nil_service_msg_callback_info callback
)
{
    auto holder = std::make_shared<ContextCleanup>(callback.context, callback.cleanup);
    as<ICallbackService>(service)->on_message(
        [exec = callback.exec, holder](const ID& id, const void* data, std::uint64_t size)
        {
            auto mutable_id = id;
            exec(
                as<nil_service_id>(&mutable_id),
                static_cast<const char*>(data),
                size,
                holder->context
            );
        }
    );
}

struct nil_service_message* nil_service_event_to_message(struct nil_service_event* service)
{
    auto* event = as<IEventService>(service);
    return as<nil_service_message>(static_cast<IMessageService*>(event));
}

struct nil_service_callback* nil_service_event_to_callback(struct nil_service_event* service)
{
    auto* event = as<IEventService>(service);
    return as<nil_service_callback>(static_cast<ICallbackService*>(event));
}

struct nil_service_event* nil_service_standalone_to_event(struct nil_service_standalone* service)
{
    auto* standalone = as<IStandaloneService>(service);
    return as<nil_service_event>(static_cast<IEventService*>(standalone));
}

struct nil_service_runnable* nil_service_standalone_to_runnable(
    struct nil_service_standalone* service
)
{
    auto* standalone = as<IStandaloneService>(service);
    return as<nil_service_runnable>(static_cast<IRunnableService*>(standalone));
}
