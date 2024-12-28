#ifndef ROLE_CHILD_HPP
#define ROLE_CHILD_HPP

#include <functional>

#include "unix/process.hpp"
#include "unix/system_v/ipc/group_notifier.hpp"

namespace role
{
    bool signal_readiness_to_parent(const unix::system_v::ipc::group_notifier
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

    bool setup_child_process(unix::process_id_t process_id,
                             const unix::system_v::ipc::group_notifier
                                 &children_readiness_notifier) noexcept
    {
        if (!signal_readiness_to_parent(children_readiness_notifier))
        {
            return false;
        }
        std::println("child process with id: {} is ready", process_id);
        return true;
    }

    class child
    {
    public:
        explicit child(unix::process_id_t process_id,
                       const unix::system_v::ipc::group_notifier
                           &children_readiness_notifier) noexcept
            : process_id_{process_id},
              children_readiness_notifier_{std::cref(children_readiness_notifier)} {}

        bool setup() const noexcept
        {
            return setup_child_process(process_id_, children_readiness_notifier_);
        }

        bool finalize() const noexcept { return true; }

    private:
        unix::process_id_t process_id_;
        std::reference_wrapper<const unix::system_v::ipc::group_notifier>
            children_readiness_notifier_;
    };
}

#endif // ROLE_CHILD_HPP