#ifndef UNIX_IPC_SYSTEM_V_GROUP_NOTIFIER_HPP
#define UNIX_IPC_SYSTEM_V_GROUP_NOTIFIER_HPP

#include <cassert>
#include <expected>

#include "unix/error_code.hpp"
#include "unix/ipc/system_v/semaphore_set.hpp"
#include "unix/utility.hpp"

namespace unix::ipc::system_v
{
    class group_notifier
    {
    public:
        explicit group_notifier(semaphore_set &semaphores,
                                semaphore_index_t semaphore_index,
                                semaphore_value_t group_size) noexcept
            : semaphores_{semaphores}, index_{semaphore_index},
              group_size_{group_size}
        {
            assert(group_size > 0);
            assert(semaphore_index < semaphores.count());
        }

        std::expected<void, error_code> wait_for_one() const noexcept
        {
            return semaphores_.decrease_value(index_, -1);
        }

        std::expected<void, error_code> wait_for(semaphore_value_t count) const noexcept
        {
            assert(count > 0);
            return semaphores_.decrease_value(index_, -count);
        }

        std::expected<void, error_code> wait_for_all() const noexcept
        {
            return semaphores_.decrease_value(index_, -group_size_);
        }

        std::expected<void, error_code> notify_one() const noexcept
        {
            return semaphores_.increase_value(index_, 1);
        }

        std::expected<void, error_code> notify(semaphore_value_t count) const noexcept
        {
            assert(count > 0);
            return semaphores_.increase_value(index_, count);
        }

        std::expected<void, error_code> notify_all() const noexcept
        {
            return semaphores_.increase_value(index_, group_size_);
        }

        std::expected<void, error_code> wait_till_none() const noexcept
        {
            return semaphores_.change_value(index_, 0);
        }

        std::expected<void, error_code> try_waiting_for_one() const noexcept
        {
            return semaphores_.try_decreasing_value(index_, -1);
        }

    private:
        semaphore_set &semaphores_;
        semaphore_index_t index_;
        semaphore_value_t group_size_;
    };
} // namespace unix::ipc::system_v

#endif // UNIX_IPC_SYSTEM_V_GROUP_NOTIFIER_HPP