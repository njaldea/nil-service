#include <nil/service/ID.hpp>

namespace nil::service
{
    std::string to_string(ID id)
    {
        return id.to_string(id.id);
    }
}
