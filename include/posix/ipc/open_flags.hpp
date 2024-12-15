#ifndef POSIX_IPC_OPEN_FLAGS_HPP
#define POSIX_IPC_OPEN_FLAGS_HPP

#include <cassert>
#include <fcntl.h>

namespace posix::ipc
{
    using open_flags_t = int;
    constexpr open_flags_t no_open_flags{0};
}

#endif // POSIX_IPC_OPEN_FLAGS_HPP