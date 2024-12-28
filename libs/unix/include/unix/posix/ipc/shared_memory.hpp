#ifndef UNIX_POSIX_IPC_SHARED_MEMORY_HPP
#define UNIX_POSIX_IPC_SHARED_MEMORY_HPP

#include <cassert>
#include <expected>
#include <string>
#include <sys/mman.h>

#include "unix/error_code.hpp"
#include "unix/posix/ipc/utility.hpp"
#include "unix/posix/ipc/primitive.hpp"


namespace unix::posix::ipc
{
    class shared_memory : public ipc::primitive
    {
    public:
        static std::expected<shared_memory, error_code> create(std::string name) noexcept
        {
            assert(is_valid_pathname(name));
        }

        std::expected<void, error_code> unlink() noexcept {
            // shm_unlink
        }
    };
}

#endif // UNIX_POSIX_IPC_SHARED_MEMORY_HPP