#ifndef UNIX_POSIX_PROCESS_HPP
#define UNIX_POSIX_PROCESS_HPP

#include <unistd.h>
#include <expected>
#include <errno.h>
#include <sys/wait.h>

#include "unix/error_code.hpp"
#include "unix/utility.hpp"

namespace unix::posix
{
    using process_id_t = pid_t;

    constexpr bool is_child_process(process_id_t pid) noexcept
    {
        return pid == 0;
    }

    std::expected<process_id_t, error_code> create_process() noexcept
    {
        const process_id_t pid = fork();

        if (!unix::operation_failed(pid))
        {
            return pid;
        }
        return std::unexpected{error_code{errno}};
    }

    process_id_t get_process_id() noexcept
    {
        return getpid();
    }

    process_id_t get_parent_process_id() noexcept
    {
        return getppid();
    }

    std::expected<process_id_t, error_code> wait_till_child_terminates(int* status) noexcept
    {
        const auto child_id = wait(status);

        if (unix::operation_failed(child_id))
        {
            return std::unexpected{error_code{errno}};
        }
        return std::expected<process_id_t, error_code>{child_id};
    }
}

#endif // UNIX_POSIX_PROCESS_HPP