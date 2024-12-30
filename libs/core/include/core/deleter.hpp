#ifndef CORE_DELETER_HPP
#define CORE_DELETER_HPP

#include <limits.h>

namespace core
{
    template <auto Function>
    struct deleter
    {
        template <class Type>
        constexpr void operator()(Type *arg) const
        {
            Function(arg);
        }
    };
}

#endif // CORE_DELETER_HPP
