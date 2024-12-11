#ifndef POSIX_PROCESS_HPP
#define POSIX_PROCESS_HPP

#include <unistd.h>
#include <expected>
#include <errno.h>

#include "posix/error_code.hpp"
#include "posix/utility.hpp"

namespace posix
{
    using process_id_t = pid_t;

    constexpr bool is_child_process(process_id_t pid) noexcept
    {
        return pid == 0;
    }

    std::expected<process_id_t, error_code> create_process() noexcept
    {
        const process_id_t pid = fork();

        if (!operation_failed(pid))
        {
            return pid;
        }
        return std::unexpected{error_code{errno}};
    }
}

#endif // POSIX_PROCESS_HPP