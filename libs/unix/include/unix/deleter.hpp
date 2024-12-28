#ifndef UNIX_DELETER_HPP
#define UNIX_DELETER_HPP

#include <limits.h>

namespace unix
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

#endif // UNIX_DELETER_HPP
