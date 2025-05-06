#include <nil/service/structs.hpp>

#include "../structs/Service.hpp"

namespace nil::service
{
    P::operator Service&() const
    {
        return *this->ptr;
    }

    P::operator MessagingService&() const
    {
        return *this->ptr;
    }

    P::operator ObservableService&() const
    {
        return *this->ptr;
    }
}
