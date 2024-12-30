#ifndef UNIX_RESOURCE_REMOVER_HPP
#define UNIX_RESOURCE_REMOVER_HPP

#include <memory>

#include "core/deleter.hpp"

#include "core/string_literal.hpp"

namespace unix
{
    template <core::string_literal ResourceName, class Resource>
    static void remove_resource(Resource *resource) noexcept
    {
        const auto removed = resource->remove();
        std::println("{} removed: {}", ResourceName.data(), removed.has_value());
        assert(removed);
    }

    template <core::string_literal ResourceName, class Resource>
    using resource_remover_t =
        std::unique_ptr<Resource,
                        core::deleter<remove_resource<ResourceName, Resource>>>;
}

#endif // UNIX_RESOURCE_REMOVER_HPP