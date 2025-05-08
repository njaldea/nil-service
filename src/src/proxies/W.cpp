#include <nil/service/structs.hpp>

#include "../structs/WebService.hpp"

namespace nil::service
{
    W::operator WebService&() const
    {
        return *this->ptr;
    }

    W::operator RunnableService&() const
    {
        return *this->ptr;
    }
}
