#ifndef UNIX_SIGNAL_HPP
#define UNIX_SIGNAL_HPP

#include <cassert>
#include <expected>
#include <signal.h>

#include "unix/error_code.hpp"
#include "unix/process.hpp"
#include "unix/utility.hpp"

namespace unix
{
    std::expected<void, error_code> send_termination_signal(process_id_t pid, int signal) noexcept
    {
        const auto ret = kill(pid, signal);

        if (operation_failed(ret))
        {
            return std::unexpected{error_code{errno}};
        }
        assert(operation_successful(ret));
        return std::expected<void, error_code>{};
    }

    std::expected<void, error_code> request_process_termination(process_id_t pid) noexcept
    {
        return send_termination_signal(pid, SIGTERM);
    }

    std::expected<void, error_code> request_process_group_termination() noexcept
    {
        return request_process_termination(process_id_t{0});
    }

    std::expected<void, error_code> request_all_accessible_process_termination() noexcept
    {
        return request_process_termination(process_id_t{-1});
    }

    std::expected<void, error_code> force_process_termination(process_id_t pid) noexcept
    {
        return send_termination_signal(pid, SIGKILL);
    }

    std::expected<void, error_code> force_process_group_termination() noexcept
    {
        return force_process_termination(process_id_t{0});
    }

    std::expected<void, error_code> force_all_accessible_process_termination() noexcept
    {
        return force_process_termination(process_id_t{-1});
    }
}

#endif // UNIX_SIGNAL_HPP
