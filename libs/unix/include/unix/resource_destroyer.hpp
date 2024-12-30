#ifndef UNIX_RESOURCE_DESTROYER_HPP
#define UNIX_RESOURCE_DESTROYER_HPP

#include <memory>

#include "core/deleter.hpp"

#include "core/string_literal.hpp"

namespace unix
{
    template <core::string_literal ResourceName, class Resource>
    static void destroy_resource(Resource *resource) noexcept
    {
        resource->~Resource();
        std::println("{} destroyed", ResourceName.data());
    }

    template <core::string_literal ResourceName, class Resource>
    using resource_destroyer_t =
        std::unique_ptr<Resource,
                        core::deleter<destroy_resource<ResourceName, Resource>>>;
}

#endif // UNIX_RESOURCE_DESTROYER_HPP