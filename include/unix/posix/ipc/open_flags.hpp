#ifndef UNIX_POSIX_IPC_OPEN_FLAGS_HPP
#define UNIX_POSIX_IPC_OPEN_FLAGS_HPP

#include <cassert>
#include <fcntl.h>

namespace unix::posix::ipc
{
    using open_flags_t = int;
    constexpr open_flags_t no_open_flags{0};
}

#endif // UNIX_POSIX_IPC_OPEN_FLAGS_HPP