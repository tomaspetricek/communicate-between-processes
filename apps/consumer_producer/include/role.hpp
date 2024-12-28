#ifndef ROLE_HPP
#define ROLE_HPP

#include "role/child.hpp"
#include "role/parent.hpp"

namespace
{
    struct no_role
    {
        bool setup() const noexcept { return true; }
        bool finalize() noexcept { return true; }
    };

    using social_role_t = std::variant<no_role, role::child, role::parent>;
} // namespace

#endif // ROLE_HPP