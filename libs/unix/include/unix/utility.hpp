#ifndef UNIX_UTILITY_HPP
#define UNIX_UTILITY_HPP

#include <limits.h>

namespace unix
{
    constexpr bool operation_successful(int ret)
    {
        return ret == 0;
    }

    constexpr bool operation_failed(int ret)
    {
        return ret == -1;
    }
}

#endif // UNIX_UTILITY_HPP
