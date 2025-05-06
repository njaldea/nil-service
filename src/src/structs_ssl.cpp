#include <nil/service/structs_ssl.hpp>

#include "structs/HTTPSService.hpp"

namespace nil::service
{
    namespace impl
    {
        void on_ready(HTTPSService& service, std::function<void(const ID&)> handler)
        {
            service.on_ready.push_back(std::move(handler));
        }
    }

    void on_get(HTTPSService& service, std::function<void(const HTTPSTransaction&)> callback)
    {
        service.on_get = std::move(callback);
    }

    P use_ws(HTTPSService& service, std::string route)
    {
        return {&service.wss[std::move(route)]};
    }
}
