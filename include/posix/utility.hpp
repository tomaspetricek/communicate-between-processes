#ifndef POSIX_UTILITY_HPP
#define POSIX_UTILITY_HPP

namespace posix
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

#endif // POSIX_UTILITY_HPP
