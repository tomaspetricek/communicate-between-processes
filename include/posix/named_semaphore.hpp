#ifndef POSIX_NAMED_SEMAPHORE_HPP
#define POSIX_NAMED_SEMAPHORE_HPP

#include "errno.h"
#include "semaphore.h"
#include <cassert>
#include <expected>
#include <fcntl.h>
#include <string_view>
#include <new>

#include "posix/error_code.hpp"
#include "posix/utility.hpp"


namespace posix
{
    enum class semaphore_open_flag
    {
        create,               // O_CREAT
        create_if_not_exists, // O_EXCL
    };

    class named_semaphore
    {
        using handle_type = sem_t;

        static int translate_flag(const semaphore_open_flag &flag) noexcept
        {
            if (flag == semaphore_open_flag::create)
            {
                return O_CREAT;
            }
            assert(flag == semaphore_open_flag::create_if_not_exists);
            return O_CREAT | O_EXCL;
        }

    public:
        explicit named_semaphore(handle_type *handle, std::string &&name) noexcept : handle_{handle}, name_{std::move(name)} {}

        static std::expected<posix::named_semaphore, error_code>
        create(std::string name, const semaphore_open_flag &flag, mode_t mode,
               unsigned int init_value) noexcept
        {
            assert(is_valid_ipc_name(name));
            handle_type *handle =
                ::sem_open(name.data(), translate_flag(flag), mode, init_value);

            if (handle != SEM_FAILED)
            {
                return std::expected<posix::named_semaphore, error_code>{std::in_place, handle, std::move(name)};
            }
            assert(handle == SEM_FAILED);
            return std::unexpected{error_code{errno}};;
        }

        named_semaphore(const named_semaphore &other) = delete;
        named_semaphore &operator=(const named_semaphore &other) = delete;

        named_semaphore(named_semaphore &&other) noexcept = delete;
        named_semaphore &operator=(named_semaphore &&other) noexcept = delete;

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
            return std::unexpected{error_code{errno}};;
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
            return std::unexpected{error_code{errno}};;
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
            return std::unexpected{error_code{errno}};;
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
            return std::unexpected{error_code{errno}};;
        }

        ~named_semaphore() noexcept
        {
            assert(close());
        }

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
            return std::unexpected{error_code{errno}};;
        }
    };
} // namespace posix

#endif // POSIX_NAMED_SEMAPHORE_HPP