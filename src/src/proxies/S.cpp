#ifdef NIL_SERVICE_SECURE

#include <nil/service/structs.hpp>

#include "../structs/HTTPSService.hpp"

namespace nil::service
{
    S::operator HTTPSService&() const
    {
        return *this->ptr;
    }

    S::operator RunnableService&() const
    {
        return *this->ptr;
    }
}

#endif
