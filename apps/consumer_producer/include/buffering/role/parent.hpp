#ifndef BUFFERING_ROLE_PARENT_HPP
#define BUFFERING_ROLE_PARENT_HPP

#include <atomic>
#include <functional>
#include <print>

#include "unix/system_v/ipc/group_notifier.hpp"
#include "unix/signal.hpp"
#include "unix/process.hpp"
#include "unix/error_code.hpp"

#include "buffering/process_info.hpp"

namespace buffering::role
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

    bool stop_consumption(
        const unix::system_v::ipc::group_notifier &message_written_notifier,
        std::atomic<bool> &done_flag) noexcept
    {
        done_flag.store(true);
        const auto stop_consumption = message_written_notifier.notify_all();

        if (!stop_consumption)
        {
            std::println("failed to not notify consumers to stop due to: {}",
                         unix::to_string(stop_consumption.error()).data());
            return false;
        }
        return true;
    }

    bool wait_till_all_messages_consumed(const unix::system_v::ipc::group_notifier
                                             &message_written_notifier) noexcept
    {
        const auto all_consumed = message_written_notifier.wait_till_none();

        if (!all_consumed)
        {
            std::println("failed waiting for all messages to be consumed due to: {}",
                         unix::to_string(all_consumed.error()).data());
            return false;
        }
        return true;
    }

    bool wait_till_production_complete(
        const unix::system_v::ipc::group_notifier &producers_notifier) noexcept
    {
        const auto production_stopped = producers_notifier.wait_for_all();

        if (!production_stopped)
        {
            std::println(
                "failed to receive confirmation that production has stopped: {}",
                unix::to_string(production_stopped.error()).data());
            return false;
        }
        return true;
    }

    bool finalize_parent_process(
        const unix::system_v::ipc::group_notifier &producers_notifier,
        const unix::system_v::ipc::group_notifier &message_written_notifier,
        const std::atomic<std::int32_t> &consumed_message_count,
        const std::atomic<std::int32_t> &produced_message_count,
        std::atomic<bool> &done_flag) noexcept
    {
        std::println("wait for all production to complete");

        if (!wait_till_production_complete(producers_notifier))
        {
            return EXIT_FAILURE;
        }
        std::println("all producers done");
        std::println("wait till all messages consumed");

        if (!wait_till_all_messages_consumed(message_written_notifier))
        {
            return EXIT_FAILURE;
        }
        std::println("all messages consumed");
        std::println("consumed message count: {}",
                     consumed_message_count.load(std::memory_order_relaxed));
        std::println("produced message count: {}",
                     produced_message_count.load(std::memory_order_relaxed));
        assert(consumed_message_count.load() == produced_message_count.load());
        std::println("stop message consumption");

        if (!stop_consumption(message_written_notifier, done_flag))
        {
            return false;
        }
        std::println("message consumption stopped");
        std::println("wait for all children to terminate");

        if (!wait_till_all_children_termninate())
        {
            return false;
        }
        std::println("all done");
        return true;
    }

    bool start_production(
        const unix::system_v::ipc::group_notifier &producers_notifier) noexcept
    {
        const auto start_production = producers_notifier.notify_all();

        if (!start_production)
        {
            std::println("failed to send start signal to producers due to: {}",
                         unix::to_string(start_production.error()).data());
            return false;
        }
        return true;
    }

    bool wait_till_all_children_ready(const std::size_t created_process_count,
                                      const unix::system_v::ipc::group_notifier
                                          &children_readiness_notifier) noexcept
    {
        using namespace unix::system_v;

        const auto readiness_signaled = children_readiness_notifier.wait_for_all();

        if (!readiness_signaled)
        {
            std::println("signal about child being ready not received due to: {}",
                         unix::to_string(readiness_signaled.error()).data());
            return false;
        }
        return true;
    }

    bool setup_parent_process(
        const process_info &info,
        const unix::system_v::ipc::group_notifier &children_readiness_notifier,
        const unix::system_v::ipc::group_notifier &producers_notifier) noexcept
    {
        std::println("wait till all children process are ready");

        if (!wait_till_all_children_ready(info.created_process_count,
                                          children_readiness_notifier))
        {
            return false;
        }
        std::println("all child processes are ready");
        std::println("start message production");

        if (!start_production(producers_notifier))
        {
            return false;
        }
        std::println("message production started");
        return true;
    }

    class parent
    {
    public:
        explicit parent(
            const process_info &info,
            const unix::system_v::ipc::group_notifier &children_readiness_notifier,
            const unix::system_v::ipc::group_notifier &producers_notifier,
            const unix::system_v::ipc::group_notifier &message_written_notifier,
            const std::atomic<std::int32_t> &consumed_message_count,
            const std::atomic<std::int32_t> &produced_message_count,
            std::atomic<bool> &done_flag) noexcept
            : info_{info},
              children_readiness_notifier_{std::cref(children_readiness_notifier)},
              producers_notifier_{std::cref(producers_notifier)},
              message_written_notifier_{std::cref(message_written_notifier)},
              consumed_message_count_{std::cref(consumed_message_count)},
              produced_message_count_{std::cref(produced_message_count)},
              done_flag_{std::ref(done_flag)} {}

        bool setup() const noexcept
        {
            return setup_parent_process(info_, children_readiness_notifier_,
                                        producers_notifier_);
        }

        bool finalize() noexcept
        {
            return finalize_parent_process(
                producers_notifier_, message_written_notifier_, consumed_message_count_,
                produced_message_count_, done_flag_);
        }

    private:
        process_info info_;
        std::reference_wrapper<const unix::system_v::ipc::group_notifier>
            children_readiness_notifier_;
        std::reference_wrapper<const unix::system_v::ipc::group_notifier>
            producers_notifier_;
        std::reference_wrapper<const unix::system_v::ipc::group_notifier>
            message_written_notifier_;
        std::reference_wrapper<const std::atomic<std::int32_t>>
            consumed_message_count_;
        std::reference_wrapper<const std::atomic<std::int32_t>>
            produced_message_count_;
        std::reference_wrapper<std::atomic<bool>> done_flag_;
    };
}

#endif // BUFFERING_ROLE_PARENT_HPP