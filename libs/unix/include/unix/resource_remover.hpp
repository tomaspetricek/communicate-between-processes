#ifndef UNIX_RESOURCE_REMOVER_HPP
#define UNIX_RESOURCE_REMOVER_HPP

#include <memory>

#include "unix/deleter.hpp"

#include "string_literal.hpp"

namespace unix
{
    template <string_literal ResourceName, class Resource>
    static void remove_resource(Resource *resource) noexcept
    {
        const auto removed = resource->remove();
        std::println("{} removed: {}", ResourceName.data(), removed.has_value());
        assert(removed);
    }

    template <string_literal ResourceName, class Resource>
    using resource_remover_t =
        std::unique_ptr<Resource,
                        unix::deleter<remove_resource<ResourceName, Resource>>>;
}

#endif // UNIX_RESOURCE_REMOVER_HPP