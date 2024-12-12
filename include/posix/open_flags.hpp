#ifndef POSIX_OPEN_FLAGS_HPP
#define POSIX_OPEN_FLAGS_HPP

#include <cassert>
#include <fcntl.h>

namespace posix
{
    using open_flags_t = int;
    constexpr open_flags_t no_open_flags{0};
}

#endif // POSIX_OPEN_FLAGS_HPP