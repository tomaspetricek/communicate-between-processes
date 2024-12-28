#ifndef BUFFERING_RESOURCE_RELEASER_HPP
#define BUFFERING_RESOURCE_RELEASER_HPP

#include <cassert>
#include <memory>
#include <print>

#include "unix/deleter.hpp"
#include "string_literal.hpp"

namespace buffering
{
    template <string_literal ResourceName, class Resource>
    static void remove_resource(Resource *resource) noexcept
    {
        const auto removed = resource->remove();
        std::println("{} removed: {}", ResourceName.data(), removed.has_value());
        assert(removed);
    }

    template <string_literal ResourceName, class Resource>
    static void destroy_resource(Resource *resource) noexcept
    {
        resource->~Resource();
        std::println("{} destroyed", ResourceName.data());
    }

    template <string_literal ResourceName, class Resource>
    using resource_remover_t =
        std::unique_ptr<Resource,
                        unix::deleter<remove_resource<ResourceName, Resource>>>;

    template <string_literal ResourceName, class Resource>
    using resource_destroyer_t =
        std::unique_ptr<Resource,
                        unix::deleter<destroy_resource<ResourceName, Resource>>>;
}

#endif // BUFFERING_RESOURCE_RELEASER_HPP