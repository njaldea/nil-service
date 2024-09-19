#include <nil/service/structs.hpp>

#include "../structs/Service.hpp"

namespace nil::service
{
    S::operator Service&() const
    {
        return *this->ptr;
    }

    S::operator MessagingService&() const
    {
        return *this->ptr;
    }

    S::operator ObservableService&() const
    {
        return *this->ptr;
    }
}
