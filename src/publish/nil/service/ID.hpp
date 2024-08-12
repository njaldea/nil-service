#pragma once

#include <functional>
#include <string>

namespace nil::service
{
    /**
     * @brief Temporary identifier for a connection
     */
    struct ID final
    {
        std::string text;
    };

    inline bool operator==(const ID& l, const ID& r)
    {
        return l.text == r.text;
    }

    inline bool operator!=(const ID& l, const ID& r)
    {
        return !(l == r);
    }
}

namespace std
{
    template <>
    struct hash<nil::service::ID>
    {
        std::size_t operator()(const nil::service::ID& id) const
        {
            return std::hash<std::string>()(id.text);
        }
    };
}
