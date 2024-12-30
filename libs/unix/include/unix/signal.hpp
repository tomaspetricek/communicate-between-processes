#ifndef UNIX_SIGNAL_HPP
#define UNIX_SIGNAL_HPP

#include <cassert>
#include <expected>
#include <signal.h>
#include <optional>

#include "unix/error_code.hpp"
#include "unix/process.hpp"
#include "unix/utility.hpp"

namespace unix
{
    using signal_handler_t = void (*)(int);
    using signal_t = int;

    constexpr process_id_t group_process_id{0};

    std::expected<void, error_code> send_termination_signal(process_id_t pid, signal_t signal) noexcept
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
        return request_process_termination(group_process_id);
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
        return force_process_termination(group_process_id);
    }

    std::expected<void, error_code> force_all_accessible_process_termination() noexcept
    {
        return force_process_termination(process_id_t{-1});
    }

    std::expected<void, error_code> interrupt_process(process_id_t pid) noexcept
    {
        return send_termination_signal(pid, SIGINT);
    }

    std::expected<void, error_code> interrupt_process_group() noexcept
    {
        return interrupt_process(group_process_id);
    }

    std::optional<signal_handler_t> register_signal_handler(signal_t sig, signal_handler_t handler) noexcept
    {
        const auto ret = signal(sig, handler);

        if (ret == SIG_ERR)
        {
            return std::nullopt;
        }
        return ret;
    }

    std::optional<signal_handler_t> register_interruption_handler(signal_handler_t handler) noexcept
    {
        return register_signal_handler(SIGINT, handler);
    }
}

#endif // UNIX_SIGNAL_HPP
