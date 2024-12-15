#ifndef POSIX_IPC_NAMED_SEMAPHORE_HPP
#define POSIX_IPC_NAMED_SEMAPHORE_HPP

#include <errno.h>
#include <semaphore.h>
#include <cassert>
#include <expected>
#include <fcntl.h>
#include <new>
#include <string_view>

#include "posix/error_code.hpp"
#include "posix/ipc/open_flags.hpp"
#include "posix/ipc/utility.hpp"
#include "posix/ipc/primitive.hpp"


namespace posix::ipc
{
    class named_semaphore : ipc::primitive
    {
        using handle_type = sem_t;

    public:
        explicit named_semaphore(handle_type *handle, std::string &&name) noexcept
            : handle_{handle}, name_{std::move(name)} {}

        static std::expected<named_semaphore, error_code>
        create(std::string name, const open_flags_t &flags, mode_t mode,
               unsigned int init_value) noexcept
        {
            assert(is_valid_pathname(name));
            handle_type *handle =
                ::sem_open(name.data(), flags, mode, init_value);

            if (handle != SEM_FAILED)
            {
                return std::expected<named_semaphore, error_code>{
                    std::in_place, handle, std::move(name)};
            }
            assert(handle == SEM_FAILED);
            return std::unexpected{error_code{errno}};
        }

        // increments the semaphore value,
        // signaling other processes waiting on the semaphore
        std::expected<void, error_code> post() noexcept
        {
            const auto ret = ::sem_post(handle_);

            if (operation_successful(ret))
            {
                return std::expected<void, error_code>{};
            }
            assert(operation_failed(ret));
            return std::unexpected{error_code{errno}};
        }

        // decrements the semaphore value
        // if the value is 0, the calling process blocks until it can decrement the
        // semaphore
        std::expected<void, error_code> wait() noexcept
        {
            const auto ret = ::sem_wait(handle_);

            if (operation_successful(ret))
            {
                return std::expected<void, error_code>{};
            }
            assert(operation_failed(ret));
            return std::unexpected{error_code{errno}};
            ;
        }

        // retrieves the current value of the semaphore
        std::expected<int, error_code> get_value() noexcept
        {
            int val;
            const auto ret = ::sem_getvalue(handle_, &val);

            if (operation_successful(ret))
            {
                return val;
            }
            assert(operation_failed(ret));
            return std::unexpected{error_code{errno}};
        }

        // removes the named semaphore from the system
        // only affects the name, not active processes using the semaphore
        std::expected<void, error_code> unlink() noexcept
        {
            const auto ret = ::sem_unlink(name_.data());

            if (operation_successful(ret))
            {
                return std::expected<void, error_code>{};
            }
            assert(operation_failed(ret));
            return std::unexpected{error_code{errno}};
        }

        ~named_semaphore() noexcept { assert(close()); }

    private:
        handle_type *handle_;
        std::string name_;

        // closes a named semaphore,
        // detaches the semaphore from the process but doesn't remove it
        std::expected<void, error_code> close() noexcept
        {
            const auto ret = ::sem_close(handle_);

            if (operation_successful(ret))
            {
                return std::expected<void, error_code>{};
            }
            assert(operation_failed(ret));
            return std::unexpected{error_code{errno}};
        }
    };
} // namespace posix::ipc

#endif // POSIX_IPC_NAMED_SEMAPHORE_HPP