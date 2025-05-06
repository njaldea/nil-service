#include <nil/service/structs.hpp>

#include "../structs/StandaloneService.hpp"

namespace nil::service
{
    A::operator StandaloneService&() const
    {
        return *this->ptr;
    }

    A::operator Service&() const
    {
        return *this->ptr;
    }

    A::operator RunnableService&() const
    {
        return *this->ptr;
    }

    A::operator MessagingService&() const
    {
        return *this->ptr;
    }

    A::operator ObservableService&() const
    {
        return *this->ptr;
    }

    A::operator P() const
    {
        return {this->ptr.get()};
    }
}
