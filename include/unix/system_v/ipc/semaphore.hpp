#ifndef UNIX_SYSTEM_V_IPC_SEMAPHORE_HPP
#define UNIX_SYSTEM_V_IPC_SEMAPHORE_HPP

#include <cassert>
#include <expected>
#include <sys/sem.h>

#include "unix/system_v/ipc/primitive.hpp"
#include "unix/system_v/ipc/key.hpp"
#include "unix/utility.hpp"

namespace unix::system_v::ipc
{
    class semaphore : public ipc::primitive
    {
        using handle_type = int;

    public:
        explicit semaphore(handle_type handle) noexcept
            : handle_{handle} {}

        static std::expected<semaphore, error_code> create(ipc::key_t key, int count, int flags) noexcept
        {
            const handle_type handle = semget(key, count, flags);

            if (!operation_failed(handle))
            {
                return std::expected<semaphore, error_code>{
                    std::in_place, handle};
            }
            return std::unexpected{error_code{errno}};
        }

    private:
        handle_type handle_;
    };
}

#endif // UNIX_SYSTEM_V_IPC_SEMAPHORE_HPP