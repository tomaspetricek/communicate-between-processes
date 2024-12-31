#ifndef UNIX_IPC_POSIX_OPEN_FLAGS_HPP
#define UNIX_IPC_POSIX_OPEN_FLAGS_HPP

#include <cassert>
#include <fcntl.h>

namespace unix::ipc::posix
{
    using open_flags_t = int;
    constexpr open_flags_t no_open_flags{0};
}

#endif // UNIX_IPC_POSIX_OPEN_FLAGS_HPP