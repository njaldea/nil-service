local ffi = require("ffi")

ffi.cdef [[
    void *malloc(size_t size);
    void free(void* ptr);
]]

ffi.cdef [[
    typedef struct nil_service_runnable
    {
        void* handle;
    } nil_service_runnable;

    typedef struct nil_service_message
    {
        void* handle;
    } nil_service_message;

    typedef struct nil_service_callback
    {
        void* handle;
    } nil_service_callback;

    typedef struct nil_service_event
    {
        void* handle;
    } nil_service_event;

    typedef struct nil_service_standalone
    {
        void* handle;
    } nil_service_standalone;

    typedef struct nil_service_gateway
    {
        void* handle;
    } nil_service_gateway;

    typedef struct nil_service_web
    {
        void* handle;
    } nil_service_web;

    typedef struct nil_service_web_transaction
    {
        void* handle;
    } nil_service_web_transaction;

    typedef struct nil_service_id
    {
        const void* owner;
        const void* id;
        void* _;
    } nil_service_id;

    typedef struct nil_service_ids
    {
        uint32_t size;
        nil_service_id* ids;
    } nil_service_ids;

    typedef struct nil_service_dispatch_info
    {
        void (*exec)(void*);
        void* context;
        void (*cleanup)(void*);
    } nil_service_dispatch_info;

    typedef struct nil_service_msg_callback_info
    {
        void (*exec)(const nil_service_id*, const void*, uint64_t, void*);
        void* context;
        void (*cleanup)(void*);
    } nil_service_msg_callback_info;

    typedef struct nil_service_callback_info
    {
        void (*exec)(const nil_service_id*, void*);
        void* context;
        void (*cleanup)(void*);
    } nil_service_callback_info;

    typedef struct nil_service_web_get_callback_info
    {
        int (*exec)(nil_service_web_transaction*, void*);
        void* context;
        void (*cleanup)(void*);
    } nil_service_web_get_callback_info;

    void nil_service_gateway_add_service(nil_service_gateway gateway, nil_service_event service);

    void nil_service_runnable_run(nil_service_runnable service);
    void nil_service_runnable_poll(nil_service_runnable service);
    void nil_service_runnable_stop(nil_service_runnable service);
    void nil_service_runnable_restart(nil_service_runnable service);
    void nil_service_runnable_dispatch(
        nil_service_runnable service,
        nil_service_dispatch_info callback
    );

    uint64_t nil_service_id_to_string(nil_service_id id, char* buffer, uint64_t size);

    void nil_service_message_publish(nil_service_message service, const void* data, uint64_t size);
    void nil_service_message_publish_ex(
        nil_service_message service,
        nil_service_id id,
        const void* data,
        uint64_t size
    );
    void nil_service_message_send(
        nil_service_message service,
        nil_service_id id,
        const void* data,
        uint64_t size
    );
    void nil_service_message_publish_ex_ids(
        nil_service_message service,
        nil_service_ids ids,
        const void* data,
        uint64_t size
    );
    void nil_service_message_send_ids(
        nil_service_message service,
        nil_service_ids ids,
        const void* data,
        uint64_t size
    );

    void nil_service_callback_on_ready(
        nil_service_callback service,
        nil_service_callback_info callback
    );

    void nil_service_callback_on_connect(
        nil_service_callback service,
        nil_service_callback_info callback
    );

    void nil_service_callback_on_disconnect(
        nil_service_callback service,
        nil_service_callback_info callback
    );

    void nil_service_callback_on_message(
        nil_service_callback service,
        nil_service_msg_callback_info callback
    );

    void nil_service_web_on_ready(nil_service_web service, nil_service_callback_info callback);

    void nil_service_web_on_get(
        nil_service_web service,
        nil_service_web_get_callback_info callback
    );

    nil_service_event nil_service_web_use_ws(nil_service_web service, const char* route);

    void nil_service_web_transaction_set_content_type(
        nil_service_web_transaction transaction,
        const char* content_type
    );

    void nil_service_web_transaction_send(
        nil_service_web_transaction transaction,
        const void* body,
        uint64_t size
    );

    const char* nil_service_web_transaction_get_route(
        nil_service_web_transaction transaction,
        uint64_t* size
    );

    nil_service_message nil_service_event_to_message(nil_service_event service);
    nil_service_callback nil_service_event_to_callback(nil_service_event service);

    nil_service_standalone nil_service_gateway_to_standalone(nil_service_gateway service);
    nil_service_event nil_service_gateway_to_event(nil_service_gateway service);
    nil_service_message nil_service_gateway_to_message(nil_service_gateway service);
    nil_service_callback nil_service_gateway_to_callback(nil_service_gateway service);
    nil_service_runnable nil_service_gateway_to_runnable(nil_service_gateway service);

    nil_service_message nil_service_standalone_to_message(nil_service_standalone service);
    nil_service_callback nil_service_standalone_to_callback(nil_service_standalone service);
    nil_service_event nil_service_standalone_to_event(nil_service_standalone service);
    nil_service_runnable nil_service_standalone_to_runnable(nil_service_standalone service);

    nil_service_runnable nil_service_web_to_runnable(nil_service_web service);

    nil_service_standalone nil_service_create_udp_client(
        const char* host,
        uint16_t port,
        uint64_t buffer
    );
    nil_service_standalone nil_service_create_udp_server(
        const char* host,
        uint16_t port,
        uint64_t buffer
    );

    nil_service_standalone nil_service_create_tcp_client(
        const char* host,
        uint16_t port,
        uint64_t buffer
    );
    nil_service_standalone nil_service_create_tcp_server(
        const char* host,
        uint16_t port,
        uint64_t buffer
    );

    nil_service_standalone nil_service_create_ws_client(
        const char* host,
        uint16_t port,
        const char* route,
        uint64_t buffer
    );
    nil_service_standalone nil_service_create_ws_server(
        const char* host,
        uint16_t port,
        const char* route,
        uint64_t buffer
    );

    nil_service_web nil_service_create_http_server(
        const char* host,
        uint16_t port,
        uint64_t buffer
    );

    nil_service_gateway nil_service_create_gateway(void);

    void nil_service_gateway_destroy(nil_service_gateway gateway);
    void nil_service_web_destroy(nil_service_web service);
    void nil_service_standalone_destroy(nil_service_standalone service);
]]

local function to_ref_id(id)
    return tonumber(ffi.cast("uintptr_t", id))
end

local function current_file_dir()
    local source = debug.getinfo(1, "S").source
    if source:sub(1, 1) == "@" then
        source = source:sub(2)
    end

    local dir = source:match("(.*/)")
    if dir == nil then
        return "./"
    end

    return dir
end

---@class nil_service.ID
---@field _owner unknown
---@field _id unknown
---@field to_string fun(self: nil_service.ID): string

---@class nil_service.Message
---@field publish fun(self: nil_service.Message, data: string)
---@field publish_ex fun(self: nil_service.Message, id: nil_service.ID, data: string)
---@field send fun(self: nil_service.Message, id: nil_service.ID, data: string)
---@field publish_ex_ids fun(self: nil_service.Message, ids: nil_service.ID[], data: string)
---@field send_ids fun(self: nil_service.Message, ids: nil_service.ID[], data: string)

---@class nil_service.Callback
---@field on_ready fun(self: nil_service.Callback, fn: fun(id: nil_service.ID))
---@field on_connect fun(self: nil_service.Callback, fn: fun(id: nil_service.ID))
---@field on_disconnect fun(self: nil_service.Callback, fn: fun(id: nil_service.ID))
---@field on_message fun(self: nil_service.Callback, fn: fun(id: nil_service.ID, data: string))

---@class nil_service.WebTransaction
---@field set_content_type fun(self: nil_service.WebTransaction, content_type: string)
---@field send fun(self: nil_service.WebTransaction, data: string)
---@field get_route fun(self: nil_service.WebTransaction): string

---@class nil_service.Event : nil_service.Message, nil_service.Callback

---@class nil_service.Runnable
---@field run fun(self: nil_service.Runnable)
---@field poll fun(self: nil_service.Runnable)
---@field stop fun(self: nil_service.Runnable)
---@field restart fun(self: nil_service.Runnable)
---@field dispatch fun(self: nil_service.Runnable, fn: fun())

---@class nil_service.Standalone : nil_service.Event, nil_service.Runnable
---@field destroy fun(self: nil_service.Standalone)

---@class nil_service.Gateway : nil_service.Standalone
---@field add_service fun(self: nil_service.Gateway, service: nil_service.Event)
---@field destroy fun(self: nil_service.Gateway)

---@class nil_service.Web : nil_service.Runnable
---@field on_ready fun(self: nil_service.Web, fn: fun(id: nil_service.ID))
---@field on_get fun(self: nil_service.Web, fn: fun(transaction: nil_service.WebTransaction): boolean)
---@field use_ws fun(self: nil_service.Web, route: string): nil_service.Event
---@field destroy fun(self: nil_service.Web)

---@class nil_service.Module
---@field create_gateway fun(): nil_service.Gateway
---@field create_udp_client fun(host: string, port: number, buffer: number): nil_service.Standalone
---@field create_udp_server fun(host: string, port: number, buffer: number): nil_service.Standalone
---@field create_tcp_client fun(host: string, port: number, buffer: number): nil_service.Standalone
---@field create_tcp_server fun(host: string, port: number, buffer: number): nil_service.Standalone
---@field create_ws_client fun(host: string, port: number, route: string, buffer: number): nil_service.Standalone
---@field create_ws_server fun(host: string, port: number, route: string, buffer: number): nil_service.Standalone
---@field create_http_server fun(host: string, port: number, buffer: number): nil_service.Web


local function add_message(lib, message, obj)
    obj._message = message
    obj.publish = function(self, data)
        local c_data = ffi.cast("const void*", data)
        lib.nil_service_message_publish(self._message, c_data, #data)
    end
    obj.publish_ex = function(self, id, data)
        local c_id = ffi.new("nil_service_id")
        c_id.owner = id._owner
        c_id.id = id._id
        local c_data = ffi.cast("const void*", data)
        lib.nil_service_message_publish_ex(self._message, c_id, c_data, #data)
    end
    obj.send = function(self, id, data)
        local c_id = ffi.new("nil_service_id")
        c_id.owner = id._owner
        c_id.id = id._id
        local c_data = ffi.cast("const void*", data)
        lib.nil_service_message_send(self._message, c_id, c_data, #data)
    end
    obj.publish_ex_ids = function(self, ids, data)
        local c_ids = ffi.new("nil_service_id[?]", #ids)
        for i, id in ipairs(ids) do
            c_ids[i - 1].owner = id._owner
            c_ids[i - 1].id = id._id
        end
        local c_ids_struct = ffi.new("nil_service_ids")
        c_ids_struct.size = #ids
        c_ids_struct.ids = c_ids
        local c_data = ffi.cast("const void*", data)
        lib.nil_service_message_publish_ex_ids(self._message, c_ids_struct, c_data, #data)
    end
    obj.send_ids = function(self, ids, data)
        local c_ids = ffi.new("nil_service_id[?]", #ids)
        for i, id in ipairs(ids) do
            c_ids[i - 1].owner = id._owner
            c_ids[i - 1].id = id._id
        end
        local c_ids_struct = ffi.new("nil_service_ids")
        c_ids_struct.size = #ids
        c_ids_struct.ids = c_ids
        local c_data = ffi.cast("const void*", data)
        lib.nil_service_message_send_ids(self._message, c_ids_struct, c_data, #data)
    end
    return obj
end

local function add_callback(fns, lib, callback, obj)
    obj._callback = callback
    obj.on_ready = function(self, fn)
        local callback_info = ffi.new("nil_service_callback_info")
        callback_info.exec = fns.callback_exec
        callback_info.cleanup = fns.cleanup
        callback_info.context = fns.store_callback(fn)
        lib.nil_service_callback_on_ready(self._callback, callback_info)
    end
    obj.on_connect = function(self, fn)
        local callback_info = ffi.new("nil_service_callback_info")
        callback_info.exec = fns.callback_exec
        callback_info.cleanup = fns.cleanup
        callback_info.context = fns.store_callback(fn)
        lib.nil_service_callback_on_connect(self._callback, callback_info)
    end
    obj.on_disconnect = function(self, fn)
        local callback_info = ffi.new("nil_service_callback_info")
        callback_info.exec = fns.callback_exec
        callback_info.cleanup = fns.cleanup
        callback_info.context = fns.store_callback(fn)
        lib.nil_service_callback_on_disconnect(self._callback, callback_info)
    end
    obj.on_message = function(self, fn)
        local callback_info = ffi.new("nil_service_msg_callback_info")
        callback_info.exec = fns.msg_callback_exec
        callback_info.cleanup = fns.cleanup
        callback_info.context = fns.store_callback(fn)
        lib.nil_service_callback_on_message(self._callback, callback_info)
    end
    return obj
end

local function add_runnable(fns, lib, runnable, obj)
    obj._runnable = runnable
    obj.run = function(self)
        lib.nil_service_runnable_run(self._runnable)
    end
    obj.poll = function(self)
        lib.nil_service_runnable_poll(self._runnable)
    end
    obj.stop = function(self)
        lib.nil_service_runnable_stop(self._runnable)
    end
    obj.restart = function(self)
        lib.nil_service_runnable_restart(self._runnable)
    end
    obj.dispatch = function(self, fn)
        local dispatch_info = ffi.new("nil_service_dispatch_info")
        dispatch_info.exec = fns.dispatch_exec
        dispatch_info.cleanup = fns.cleanup
        dispatch_info.context = fns.store_callback(fn)
        lib.nil_service_runnable_dispatch(self._runnable, dispatch_info)
    end
    return obj
end

local function add_event(fns, lib, event, obj)
    obj._event = event
    add_message(lib, lib.nil_service_event_to_message(event), obj)
    add_callback(fns, lib, lib.nil_service_event_to_callback(event), obj)
    return obj
end

local function create_web_transaction(lib, transaction)
    return {
        _transaction = transaction,
        set_content_type = function(self, content_type)
            lib.nil_service_web_transaction_set_content_type(self._transaction, content_type)
        end,
        send = function(self, data)
            local c_data = ffi.cast("const void*", data)
            lib.nil_service_web_transaction_send(self._transaction, c_data, #data)
        end,
        get_route = function(self)
            local size = ffi.new("uint64_t[1]")
            local route = lib.nil_service_web_transaction_get_route(self._transaction, size)
            return ffi.string(route, size[0])
        end
    }
end

local function create_web(fns, lib, web)
    local obj = {
        _web = web,
        on_ready = function(self, fn)
            local callback_info = ffi.new("nil_service_callback_info")
            callback_info.exec = fns.callback_exec
            callback_info.cleanup = fns.cleanup
            callback_info.context = fns.store_callback(fn)
            lib.nil_service_web_on_ready(self._web, callback_info)
        end,
        on_get = function(self, fn)
            local callback_info = ffi.new("nil_service_web_get_callback_info")
            callback_info.exec = fns.web_get_callback_exec
            callback_info.cleanup = fns.cleanup
            callback_info.context = fns.store_callback(fn)
            lib.nil_service_web_on_get(self._web, callback_info)
        end,
        use_ws = function(self, route)
            local event_obj = {}
            add_event(fns, lib, lib.nil_service_web_use_ws(self._web, route), event_obj)
            return event_obj
        end,
        destroy = function(self)
            lib.nil_service_web_destroy(self._web)
        end
    }
    add_runnable(fns, lib, lib.nil_service_web_to_runnable(web), obj)
    return obj
end


local function create_standalone(fns, lib, standalone)
    local obj = {
        _standalone = standalone,
        destroy = function(self)
            lib.nil_service_standalone_destroy(self._standalone)
        end
    }
    add_event(fns, lib, lib.nil_service_standalone_to_event(standalone), obj)
    add_runnable(fns, lib, lib.nil_service_standalone_to_runnable(standalone), obj)
    return obj
end

local function create_gateway(fns, lib, gateway)
    local obj = {
        _gateway = gateway,
        add_service = function(self, service)
            lib.nil_service_gateway_add_service(self._gateway, service._event)
        end,
        destroy = function(self)
            lib.nil_service_gateway_destroy(self._gateway)
        end
    }
    add_event(fns, lib, lib.nil_service_gateway_to_event(gateway), obj)
    add_runnable(fns, lib, lib.nil_service_gateway_to_runnable(gateway), obj)
    return obj
end

local function create_lib_fns(refs, lib)
    local callback_exec = ffi.cast(
        "void (*)(const nil_service_id*, void*)",
        function(id, ctx_id)
            local callback_data = refs[to_ref_id(ctx_id)]
            if callback_data then
                local id_obj = {
                    _owner = id.owner,
                    _id = id.id,
                    to_string = function(self)
                        local buf = ffi.new("char[256]")
                        local n = lib.nil_service_id_to_string(id[0], buf, 256)
                        return ffi.string(buf, n)
                    end,
                }
                callback_data(id_obj)
            end
        end
    )

    local msg_callback_exec = ffi.cast(
        "void (*)(const nil_service_id*, const void*, uint64_t, void*)",
        function(id, data, size, ctx_id)
            local callback_data = refs[to_ref_id(ctx_id)]
            if callback_data then
                local id_obj = {
                    _owner = id.owner,
                    _id = id.id,
                    to_string = function(self)
                        local buf = ffi.new("char[256]")
                        local n = lib.nil_service_id_to_string(id[0], buf, 256)
                        return ffi.string(buf, n)
                    end,
                }
                callback_data(id_obj, ffi.string(data, size))
            end
        end
    )

    local dispatch_exec = ffi.cast(
        "void (*)(void*)",
        function(ctx_id)
            local callback_data = refs[to_ref_id(ctx_id)]
            if callback_data then
                callback_data()
            end
        end
    )

    local web_get_callback_exec = ffi.cast(
        "int (*)(nil_service_web_transaction*, void*)",
        function(transaction, ctx_id)
            local callback_data = refs[to_ref_id(ctx_id)]
            if callback_data then
                local trans_obj = create_web_transaction(lib, transaction)
                local result = callback_data(trans_obj)
                return result and 1 or 0
            end
            return 0
        end
    )

    local cleanup = ffi.cast(
        "void (*)(void*)",
        function(ctx_id)
            if ctx_id ~= nil then
                refs[to_ref_id(ctx_id)] = nil
                ffi.C.free(ctx_id)
            end
        end
    )

    local store_callback = function(data)
        local id = ffi.C.malloc(1)
        refs[to_ref_id(id)] = data
        return id
    end

    return {
        callback_exec = callback_exec,
        msg_callback_exec = msg_callback_exec,
        dispatch_exec = dispatch_exec,
        web_get_callback_exec = web_get_callback_exec,
        cleanup = cleanup,
        store_callback = store_callback,
    }
end

local function load_library()
    local lib_path = current_file_dir() .. "libnil-service-c-api.so"
    return ffi.load(lib_path)
end

local function create_service_lib()
    local lib = load_library()
    local refs = {}
    local fns = create_lib_fns(refs, lib)

    return {
        create_gateway = function()
            local gateway = lib.nil_service_create_gateway()
            return create_gateway(fns, lib, gateway)
        end,

        create_udp_client = function(host, port, buffer)
            local standalone = lib.nil_service_create_udp_client(host, port, buffer)
            return create_standalone(fns, lib, standalone)
        end,

        create_udp_server = function(host, port, buffer)
            local standalone = lib.nil_service_create_udp_server(host, port, buffer)
            return create_standalone(fns, lib, standalone)
        end,

        create_tcp_client = function(host, port, buffer)
            local standalone = lib.nil_service_create_tcp_client(host, port, buffer)
            return create_standalone(fns, lib, standalone)
        end,

        create_tcp_server = function(host, port, buffer)
            local standalone = lib.nil_service_create_tcp_server(host, port, buffer)
            return create_standalone(fns, lib, standalone)
        end,

        create_ws_client = function(host, port, route, buffer)
            local standalone = lib.nil_service_create_ws_client(host, port, route, buffer)
            return create_standalone(fns, lib, standalone)
        end,

        create_ws_server = function(host, port, route, buffer)
            local standalone = lib.nil_service_create_ws_server(host, port, route, buffer)
            return create_standalone(fns, lib, standalone)
        end,

        create_http_server = function(host, port, buffer)
            local web = lib.nil_service_create_http_server(host, port, buffer)
            return create_web(fns, lib, web)
        end,
    }
end

return create_service_lib()
