#pragma once

#include <functional>
#include <string>

namespace nil::service
{
    struct ID final
    {
        // NOLINTNEXTLINE
        ID(const void* init_owner, const void* init_id, std::string (*init_to_string)(const void*))
            : owner(init_owner)
            , id(init_id)
            , to_string(init_to_string)
        {
        }

        ID() = default;
        ~ID() = default;

        ID(const ID& o) = default;
        ID& operator=(const ID& o) = default;

        ID(ID&& o) = delete;
        ID& operator=(ID&& o) = delete;

        bool operator==(const ID& o) const
        {
            return owner == o.owner && id == o.id;
        }

        bool operator!=(const ID& o) const
        {
            return !(*this == o);
        }

        const void* owner = nullptr;
        const void* id = nullptr;
        std::string (*to_string)(const void*) = nullptr;
    };

    std::string to_string(const ID& id);
}

namespace std
{
    template <>
    struct hash<nil::service::ID>
    {
        std::size_t operator()(const nil::service::ID& id) const
        {
            return std::hash<const void*>()(id.id);
        }
    };
}
