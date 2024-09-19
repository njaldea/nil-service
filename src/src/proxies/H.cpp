#include <nil/service/structs.hpp>

#include "../structs/HTTPService.hpp"

namespace nil::service
{
    H::operator HTTPService&() const
    {
        return *this->ptr;
    }

    H::operator RunnableService&() const
    {
        return *this->ptr;
    }
}
