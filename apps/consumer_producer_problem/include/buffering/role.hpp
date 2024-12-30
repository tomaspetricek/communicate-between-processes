#ifndef BUFFERING_ROLE_HPP
#define BUFFERING_ROLE_HPP

#include <variant>

#include "buffering/role/child.hpp"
#include "buffering/role/parent.hpp"

namespace buffering
{
    struct no_role
    {
        bool setup() const noexcept { return true; }
        bool finalize() noexcept { return true; }
    };

    using role_t = std::variant<no_role, role::child, role::parent>;
} // namespace

#endif // BUFFERING_ROLE_HPP