#ifndef COMMON_PROCESS_HPP
#define COMMON_PROCESS_HPP

#include <print>

#include "unix/ipc/system_v/group_notifier.hpp"
#include "unix/process.hpp"
#include "unix/signal.hpp"

namespace common
{
    bool wait_till_all_children_termninate() noexcept
    {
        int status;

        while (true)
        {
            const auto child_terminated = unix::wait_till_child_terminates(&status);

            if (!child_terminated)
            {
                const auto error = child_terminated.error();

                if (error.code != ECHILD)
                {
                    std::println("failed waiting for a child: {}",
                                 unix::to_string(error).data());
                    return false;
                }
                std::println("all children terminated");
                break;
            }
            const auto child_id = child_terminated.value();

            std::println("child with process id: {} terminated", child_id);

            if (WIFEXITED(status))
            {
                std::println("child exit status: {}", WEXITSTATUS(status));
            }
            else if (WIFSIGNALED(status))
            {
                psignal(WTERMSIG(status), "child exit signal");
            }
        }
        return true;
    }

    bool wait_till_all_children_ready(const unix::ipc::system_v::group_notifier
                                          &children_readiness_notifier) noexcept
    {
        const auto readiness_signaled = children_readiness_notifier.wait_for_all();

        if (!readiness_signaled)
        {
            std::println("signal about child being ready not received due to: {}",
                         unix::to_string(readiness_signaled.error()).data());
            return false;
        }
        return true;
    }

    bool signal_readiness_to_parent(const unix::ipc::system_v::group_notifier
                                        &children_readiness_notifier) noexcept
    {
        const auto readiness_signaled = children_readiness_notifier.notify_one();

        if (!readiness_signaled)
        {
            std::println("failed to send signal about readiness due to: {}",
                         unix::to_string(readiness_signaled.error()).data());
            return false;
        }
        return true;
    }
}

#endif // COMMON_PROCESS_HPP