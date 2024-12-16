#ifndef UNIX_SYSTEM_V_IPC_SEMAPHORE_HPP
#define UNIX_SYSTEM_V_IPC_SEMAPHORE_HPP

#include <array>
#include <cassert>
#include <expected>
#include <sys/sem.h>
#include <span>

#include "unix/system_v/ipc/primitive.hpp"
#include "unix/system_v/ipc/key.hpp"
#include "unix/utility.hpp"
#include "unix/error_code.hpp"

#include <print>

namespace unix::system_v::ipc
{
    class semaphore_set : public ipc::primitive
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

        static std::expected<semaphore_set, error_code> create(ipc::key_t key, int semaphore_count, int flags) noexcept
        {
            const handle_type handle = semget(key, semaphore_count, flags);

            if (operation_failed(handle))
            {
                return std::unexpected{error_code{errno}};
            }
            const auto info_retrived = get_info(handle);

            if (!info_retrived) [[unlikely]]
            {
                return std::unexpected{info_retrived.error()};
            }
            std::println("created");
            const auto count = info_retrived.value().sem_nsems;
            assert(count > 0);
            return std::expected<semaphore_set, error_code>{std::in_place, handle, count};
        }

        static std::expected<semaphore_set, error_code> open_existing(ipc::key_t key) noexcept
        {
            assert(key > ipc::key_t{0});
            return create(key, int{0}, int{0});
        }

        static std::expected<semaphore_set, error_code> create_if_absent(ipc::key_t key, int semaphore_count, int permissions) noexcept
        {
            assert(key > ipc::key_t{0});
            assert(semaphore_count > int{0});
            return create(key, semaphore_count, IPC_CREAT | permissions);
        }

        static std::expected<semaphore_set, error_code> create_exclusively(ipc::key_t key, int semaphore_count, int permissions) noexcept
        {
            assert(key > ipc::key_t{0});
            assert(semaphore_count > int{0});
            return create(key, semaphore_count, IPC_CREAT | IPC_EXCL | permissions);
        }

        static std::expected<semaphore_set, error_code> create_private(int semaphore_count, int permissions) noexcept
        {
            assert(semaphore_count > int{0});
            return create(get_private_key(), semaphore_count, permissions);
        }

        std::expected<void, error_code> set_value(int sem_index, int init_value) noexcept
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

        std::expected<void, error_code> set_values(const std::span<unsigned short> &init_values) noexcept
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

        std::expected<void, error_code> get_value(int sem_index, int &current_value)
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

        std::expected<void, error_code> get_values(std::span<unsigned short> current_values)
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

    private:
        handle_type handle_;
        int count_;
    };
}

#endif // UNIX_SYSTEM_V_IPC_SEMAPHORE_HPP