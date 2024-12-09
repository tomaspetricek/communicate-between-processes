#ifndef POSIX_PROCESS_HPP
#define POSIX_PROCESS_HPP

#include <unistd.h>

namespace posix {
    constexpr bool is_child_process(pid_t pid) noexcept
    {
        return pid == 0;
    }
}

#endif // POSIX_PROCESS_HPP