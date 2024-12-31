#ifndef BUFFERING_ROLE_CHILD_HPP
#define BUFFERING_ROLE_CHILD_HPP

#include <functional>
#include <print>

#include "unix/process.hpp"
#include "unix/ipc/system_v/group_notifier.hpp"

#include "common/process.hpp"

namespace buffering::role
{
    bool setup_child_process(unix::process_id_t process_id,
                             const unix::ipc::system_v::group_notifier
                                 &children_readiness_notifier) noexcept
    {
        if (!common::signal_readiness_to_parent(children_readiness_notifier))
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
                       const unix::ipc::system_v::group_notifier
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
        std::reference_wrapper<const unix::ipc::system_v::group_notifier>
            children_readiness_notifier_;
    };
}

#endif // BUFFERING_ROLE_CHILD_HPP