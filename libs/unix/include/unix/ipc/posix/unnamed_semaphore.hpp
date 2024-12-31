#ifndef UNIX_IPC_POSIX_UNNAMED_SEMAPHORE_HPP
#define UNIX_IPC_POSIX_UNNAMED_SEMAPHORE_HPP

#include <cstdint>
#include <errno.h>
#include <expected>

#include "semaphore.h"

#include "unix/error_code.hpp"
#include "unix/ipc/posix/primitive.hpp"
#include "unix/utility.hpp"


namespace unix::ipc::posix
{
    enum class shared_between : std::uint8_t
    {
        threads,
        processes,
    };

    class unnamed_semaphore : public primitive
    {
        using handle_type = sem_t;

    public:
        // find a way to make it private
        explicit unnamed_semaphore(const handle_type &handle) noexcept : handle_{handle} {}

        static std::expected<unnamed_semaphore, error_code> create(shared_between shared, unsigned int init_value) noexcept
        {
            handle_type handle;
            const auto ret = ::sem_init(&handle, static_cast<int>(shared), init_value);

            if (unix::operation_successful(ret))
            {
                return std::expected<unnamed_semaphore, error_code>{std::in_place, handle};
            }
            assert(unix::operation_failed(ret));
            return std::unexpected{error_code{errno}};
        }

        std::expected<void, error_code> wait() noexcept
        {
            // wait until the semaphore value > 0 and decrement it
            const auto ret = ::sem_wait(&handle_);

            if (unix::operation_successful(ret))
            {
                return std::expected<void, error_code>{};
            }
            assert(unix::operation_failed(ret));
            return std::unexpected{error_code{errno}};
        }

        std::expected<void, error_code> post() noexcept
        {
            const auto ret = ::sem_post(&handle_);

            if (unix::operation_successful(ret))
            {
                return std::expected<void, error_code>{};
            }
            assert(unix::operation_failed(ret));
            return std::unexpected{error_code{errno}};
        }

        std::expected<void, error_code> destroy() noexcept {
            const auto ret = ::sem_destroy(&handle_);

            if (unix::operation_successful(ret))
            {
                return std::expected<void, error_code>{};
            }
            assert(unix::operation_failed(ret));
            return std::unexpected{error_code{errno}};
        }

    private:
        handle_type handle_;
    };
}

#endif // UNIX_IPC_POSIX_UNNAMED_SEMAPHORE_HPP