#ifndef UNIX_SYSTEM_V_IPC_SHARED_MEMORY_HPP
#define UNIX_SYSTEM_V_IPC_SHARED_MEMORY_HPP

#include <cassert>
#include <expected>
#include <sys/shm.h>
#include <memory>

#include "unix/deleter.hpp"
#include "unix/error_code.hpp"
#include "unix/utility.hpp"
#include "unix/system_v/ipc/key.hpp"
#include "unix/system_v/ipc/primitive.hpp"

namespace unix::system_v::ipc
{
    static void detach_shared_memory(void *shm_ptr) noexcept
    {
        const auto ret = shmdt(shm_ptr);
        assert(!operation_failed(ret));
    }

    using shared_memory_ptr_t = std::unique_ptr<void, deleter<detach_shared_memory>>;

    class shared_memory : public ipc::primitive
    {
        using handle_type = int;

    public:
        explicit shared_memory(handle_type handle) noexcept
            : handle_{handle} {}

        static std::expected<shared_memory, error_code> create(ipc::key_t key, size_t size, int flags) noexcept
        {
            const auto handle = shmget(key, size, flags);

            if (operation_failed(handle))
            {
                return std::unexpected{error_code{errno}};
            }
            return std::expected<shared_memory, error_code>{std::in_place, handle};
        }

        static std::expected<shared_memory, error_code> open_existing(ipc::key_t key) noexcept
        {
            return create(key, int{0}, int{0});
        }

        static std::expected<shared_memory, error_code> create_if_absent(ipc::key_t key, size_t size, int permissions) noexcept
        {
            assert(key > ipc::key_t{0});
            return create(key, size, IPC_CREAT | permissions);
        }

        static std::expected<shared_memory, error_code> create_exclusively(ipc::key_t key, size_t size, int permissions) noexcept
        {
            assert(key > ipc::key_t{0});
            return create(key, size, IPC_CREAT | IPC_EXCL | permissions);
        }

        static std::expected<shared_memory, error_code> create_private(size_t size, int permissions) noexcept
        {
            return create(get_private_key(), size, permissions);
        }

        std::expected<shared_memory_ptr_t, error_code> attach(const void *attachment_address, int flags) const noexcept
        {
            auto *ret = shmat(handle_, attachment_address, flags);

            if (ret == (void *)-1)
            {
                return std::unexpected{error_code{errno}};
            }
            return shared_memory_ptr_t{ret};
        }

        std::expected<shared_memory_ptr_t, error_code> attach_anywhere(int flags) const noexcept
        {
            return attach(nullptr, flags);
        }

        std::expected<void, error_code> remove() const noexcept
        {
            const auto ret = shmctl(handle_, IPC_RMID, nullptr);

            if (operation_failed(ret))
            {
                return std::unexpected{error_code{errno}};
            }
            return std::expected<void, error_code>{};
        }

    private:
        handle_type handle_;
    };
}

#endif // UNIX_SYSTEM_IPC_V_SHARED_MEMORY_HPP