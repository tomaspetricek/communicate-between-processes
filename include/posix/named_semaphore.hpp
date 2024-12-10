#ifndef POSIX_NAMED_SEMAPHORE_HPP
#define POSIX_NAMED_SEMAPHORE_HPP

#include "errno.h"
#include "semaphore.h"
#include <cassert>
#include <expected>
#include <fcntl.h>
#include <string_view>
#include <new>

#include "error_code.hpp"

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
        create(const std::string_view &name, const semaphore_open_flag &flag, mode_t mode,
               unsigned int init_value) noexcept
        {
            handle_type *handle =
                ::sem_open(name.data(), translate_flag(flag), mode, init_value);

            if (handle != SEM_FAILED)
            {
                try
                {
                    auto sem_name = std::string{name};
                    return std::expected<posix::named_semaphore, error_code>{std::in_place,
                                                                             handle, std::move(sem_name)};
                }
                catch (std::bad_alloc &)
                {
                    return std::unexpected{error_code::bad_alloc};
                }
            }
            assert(handle == SEM_FAILED);

            if (errno == EACCES)
            {
                // insufficient permissions to create/open the semaphore
                return std::unexpected{error_code::insufficient_permissions};
            }
            if (errno == EEXIST)
            {
                // semaphore already exists and O_CREAT | O_EXCL was used
                return std::unexpected{error_code::already_exists};
            }
            if (errno == EINVAL)
            {
                // invalid semaphore name or value
                return std::unexpected{error_code::invalid_argument};
            }
            if (errno == EMFILE)
            {
                // too many file descriptors open
                return std::unexpected{error_code::process_max_open_file_limit_reached};
            }
            if (errno == ENAMETOOLONG)
            {
                // semaphore name is too long
                return std::unexpected{error_code::name_too_long};
            }
            if (errno == ENFILE)
            {
                // system-wide file descriptor table is full
                return std::unexpected{error_code::system_max_open_file_limit_reached};
            }
            assert(errno == ENOENT);
            // semaphore does not exist, and O_CREAT was not used
            return std::unexpected{error_code::not_exists};
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

            if (ret == 0)
            {
                return std::expected<void, error_code>{};
            }
            assert(ret == -1);

            if (errno == EINVAL)
            {
                // semaphore is not valid
                return std::unexpected{error_code::is_invalid};
            }
            assert(errno == EOVERFLOW);
            // semaphore value exceeds the maximum limit
            return std::unexpected{error_code::overflow};
        }

        // decrements the semaphore value
        // if the value is 0, the calling process blocks until it can decrement the
        // semaphore
        std::expected<void, error_code> wait() noexcept
        {
            const auto ret = ::sem_wait(handle_);

            if (ret == 0)
            {
                return std::expected<void, error_code>{};
            }
            assert(ret == -1);

            if (errno == EINVAL)
            {
                // semaphore is not valid
                return std::unexpected{error_code::is_invalid};
            }
            assert(errno == EINTR);
            // wait was interrupted by a signal
            return std::unexpected{error_code::interrupted};
        }

        // retrieves the current value of the semaphore
        std::expected<int, error_code> get_value() noexcept
        {
            int val;
            const auto ret = ::sem_getvalue(handle_, &val);

            if (ret == 0)
            {
                return val;
            }
            assert(ret == -1);
            assert(errno == EINVAL);
            // semaphore is not valid
            return std::unexpected{error_code::is_invalid};
        }

        ~named_semaphore() noexcept
        {
            assert(close());
            assert(unlink());
        }

    private:
        handle_type *handle_;
        std::string name_;

        // closes a named semaphore,
        // detaches the semaphore from the process but doesn't remove it
        std::expected<void, error_code> close() noexcept
        {
            const auto ret = ::sem_close(handle_);

            if (ret == 0)
            {
                return std::expected<void, error_code>{};
            }
            assert(ret == -1);
            assert(errno == EINVAL);
            // semaphore is not valid
            return std::unexpected{error_code::is_invalid};
        }

        // removes the named semaphore from the system
        // only affects the name, not active processes using the semaphore
        std::expected<void, error_code> unlink() noexcept
        {
            const auto ret = ::sem_unlink(name_.data());

            if (ret == 0)
            {
                return std::expected<void, error_code>{};
            }
            assert(ret == -1);

            if (errno == EACCES)
            {
                // insufficient permissions to unlink the semaphore
                return std::unexpected{error_code::insufficient_permissions};
            }
            assert(errno == ENOENT);
            // named semaphore does not exist
            return std::unexpected{error_code::not_exists};
        }
    };
} // namespace posix

#endif // POSIX_NAMED_SEMAPHORE_HPP