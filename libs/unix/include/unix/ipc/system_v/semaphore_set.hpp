#ifndef UNIX_IPC_SYSTEM_V_SEMAPHORE_HPP
#define UNIX_IPC_SYSTEM_V_SEMAPHORE_HPP

#include <array>
#include <cassert>
#include <expected>
#include <sys/sem.h>
#include <span>

#include "unix/ipc/system_v/primitive.hpp"
#include "unix/ipc/system_v/key.hpp"
#include "unix/utility.hpp"
#include "unix/error_code.hpp"

namespace unix::ipc::system_v
{
    using semaphore_value_t = int;
    using semaphore_index_t = semaphore_value_t;

    class semaphore_set : public primitive
    {
        using handle_type = int;

        union semun
        {
            int val;
            struct semid_ds *buf;
            unsigned short *array;
        };

        static std::expected<semid_ds, error_code> get_info(handle_type handle) noexcept
        {
            struct semid_ds info;
            union semun arg;
            arg.buf = &info;

            const auto ret = semctl(handle, 0, IPC_STAT, arg);

            if (operation_failed(ret))
            {
                return std::unexpected{error_code{errno}};
            }
            return std::expected<semid_ds, error_code>{info};
        }

    public:
        explicit semaphore_set(handle_type handle, int count) noexcept
            : handle_{handle}, count_{count} {}

        std::expected<semid_ds, error_code> get_info() const noexcept
        {
            return semaphore_set::get_info(handle_);
        }

        static std::expected<semaphore_set, error_code> create(system_v::key_t key, int semaphore_count, int flags) noexcept
        {
            const handle_type handle = semget(key, semaphore_count, flags);

            if (operation_failed(handle))
            {
                return std::unexpected{error_code{errno}};
            }
            const auto info_retrieved = get_info(handle);

            if (!info_retrieved) [[unlikely]]
            {
                return std::unexpected{info_retrieved.error()};
            }
            const auto count = info_retrieved.value().sem_nsems;
            assert(count > 0);
            return std::expected<semaphore_set, error_code>{std::in_place, handle, count};
        }

        static std::expected<semaphore_set, error_code> open_existing(system_v::key_t key) noexcept
        {
            assert(key > system_v::key_t{0});
            return create(key, int{0}, int{0});
        }

        static std::expected<semaphore_set, error_code> create_if_absent(system_v::key_t key, int semaphore_count, int permissions) noexcept
        {
            assert(key > system_v::key_t{0});
            assert(semaphore_count > int{0});
            return create(key, semaphore_count, IPC_CREAT | permissions);
        }

        static std::expected<semaphore_set, error_code> create_exclusively(system_v::key_t key, int semaphore_count, int permissions) noexcept
        {
            assert(key > system_v::key_t{0});
            assert(semaphore_count > int{0});
            return create(key, semaphore_count, IPC_CREAT | IPC_EXCL | permissions);
        }

        static std::expected<semaphore_set, error_code> create_private(int semaphore_count, int permissions) noexcept
        {
            assert(semaphore_count > int{0});
            return create(get_private_key(), semaphore_count, permissions);
        }

        std::expected<void, error_code> set_value(semaphore_index_t sem_index, semaphore_value_t init_value) const noexcept
        {
            assert(sem_index >= int{0});
            union semun arg;
            arg.val = init_value;

            const auto ret = semctl(handle_, sem_index, SETVAL, arg);

            if (!operation_failed(ret))
            {
                return std::expected<void, error_code>{};
            }
            return std::unexpected{error_code{errno}};
        }

        std::expected<void, error_code> set_values(const std::span<unsigned short> &init_values) const noexcept
        {
            // otherwise overflow
            if (init_values.size() < count_)
            {
                return std::unexpected{error_code{EINVAL}};
            }
            union semun arg;
            arg.array = init_values.data();
            const auto ret = semctl(handle_, 0, SETALL, arg);

            if (!operation_failed(ret))
            {
                return std::expected<void, error_code>{};
            }
            return std::unexpected{error_code{errno}};
        }

        std::expected<void, error_code> get_value(semaphore_index_t sem_index, semaphore_value_t &current_value) const noexcept
        {
            assert(sem_index >= int{0});
            union semun arg;
            const auto ret = semctl(handle_, sem_index, GETVAL, arg);

            if (!operation_failed(ret))
            {
                current_value = arg.val;
                return std::expected<void, error_code>{};
            }
            return std::unexpected{error_code{errno}};
        }

        std::expected<void, error_code> get_values(std::span<unsigned short> current_values) const noexcept
        {
            // otherwise overflow
            if (current_values.size() < count_)
            {
                return std::unexpected{error_code{EINVAL}};
            }
            union semun arg;
            arg.array = current_values.data();
            const auto ret = semctl(handle_, 0, GETALL, arg);

            if (!operation_failed(ret))
            {
                // set directly into the buffer no need for making a copy
                return std::expected<void, error_code>{};
            }
            return std::unexpected{error_code{errno}};
        }

        int count() const noexcept
        {
            return count_;
        }

        std::expected<void, error_code> change_values(const std::span<sembuf> &ops) const noexcept
        {
            assert(ops.size() > 0);

            const auto ret = ::semop(handle_, ops.data(), ops.size());

            if (operation_failed(ret))
            {
                return std::unexpected{error_code{errno}};
            }
            return std::expected<void, error_code>{};
        }

#ifndef __APPLE__
        std::expected<void, error_code> change_values(const std::span<sembuf> &ops, const timespec &timeout) const noexcept
        {
            assert(ops.size() > 0);

            const auto ret = ::semtimedop(handle_, ops.data(), ops.size(), timeout);

            if (operation_failed(ret))
            {
                return std::unexpected{error_code{errno}};
            }
            return std::expected<void, error_code>{};
        }
#endif

        std::expected<void, error_code> change_value(semaphore_index_t sem_index, semaphore_value_t change) const noexcept
        {
            assert(sem_index >= 0 && sem_index < count_);
            sembuf args;
            args.sem_num = sem_index;
            args.sem_op = change;
            args.sem_flg = 0;

            const auto ret = semop(handle_, &args, 1);

            if (operation_failed(ret))
            {
                return std::unexpected{error_code{errno}};
            }
            return std::expected<void, error_code>{};
        }

        std::expected<void, error_code> increase_value(semaphore_index_t sem_index, semaphore_value_t increment) const noexcept
        {
            assert(sem_index >= 0 && sem_index < count_);
            assert(increment > 0);
            return change_value(sem_index, increment);
        }

        std::expected<void, error_code> decrease_value(semaphore_index_t sem_index, semaphore_value_t decrement) const noexcept
        {
            assert(sem_index >= 0 && sem_index < count_);
            assert(decrement < 0);
            return change_value(sem_index, decrement);
        }

        std::expected<void, error_code> try_changing_value(semaphore_index_t sem_index, semaphore_value_t change) const noexcept
        {
            assert(sem_index >= 0 && sem_index < count_);
            sembuf args;
            args.sem_num = sem_index;
            args.sem_op = change;
            args.sem_flg = IPC_NOWAIT;

            const auto ret = semop(handle_, &args, 1);

            if (operation_failed(ret))
            {
                return std::unexpected{error_code{errno}};
            }
            return std::expected<void, error_code>{};
        }

        std::expected<void, error_code> try_increasing_value(semaphore_index_t sem_index, semaphore_value_t increment) const noexcept
        {
            assert(sem_index >= 0 && sem_index < count_);
            assert(increment > 0);
            return try_changing_value(sem_index, increment);
        }

        std::expected<void, error_code> try_decreasing_value(semaphore_index_t sem_index, semaphore_value_t decrement) const noexcept
        {
            assert(sem_index >= 0 && sem_index < count_);
            assert(decrement < 0);
            return try_changing_value(sem_index, decrement);
        }

        std::expected<void, error_code> remove() const noexcept
        {
            const auto ret = semctl(handle_, 0, IPC_RMID);

            if (operation_failed(ret))
            {
                return std::unexpected{error_code{errno}};
            }
            return std::expected<void, error_code>{};
        }

    private:
        handle_type handle_;
        int count_;
    };
}

#endif // UNIX_IPC_SYSTEM_V_SEMAPHORE_HPP