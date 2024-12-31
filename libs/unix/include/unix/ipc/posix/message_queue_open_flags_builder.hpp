#ifndef UNIX_IPC_POSIX_MESSAGE_QUEUE_OPEN_FLAGS_BUILDER
#define UNIX_IPC_POSIX_MESSAGE_QUEUE_OPEN_FLAGS_BUILDER

#include <array>
#include <cassert>

#include "unix/ipc/posix/open_flags.hpp"
#include "unix/ipc/posix/open_flags_builder.hpp"

namespace unix::ipc::posix
{
    enum class access_mode
    {
        read_only,  // O_RDONLY
        write_only, // O_WRONLY
        read_write, // O_RDWR
    };

    class message_queue_open_flags_builder : public open_flags_builder<message_queue_open_flags_builder>
    {
    public:
        constexpr explicit message_queue_open_flags_builder(access_mode access) noexcept
        {
            constexpr std::size_t access_mode_count = 3;
            const auto access_index = static_cast<std::size_t>(access);
            assert(access_index >= std::size_t{0});
            assert(access_index < access_mode_count);
            const auto access_flags = std::array<open_flags_t, access_mode_count>{
                O_RDONLY, O_WRONLY, O_RDWR};
            flags_ |= access_flags[access_index];
        }

        constexpr message_queue_open_flags_builder &is_blocking() noexcept
        {
            // default mode
            return *this;
        }

        constexpr message_queue_open_flags_builder &is_non_blocking() noexcept
        {
            flags_ |= O_NONBLOCK;
            return *this;
        }
    };
} // namespace unix::ipc::posix

#endif // UNIX_IPC_POSIX_MESSAGE_QUEUE_OPEN_FLAGS_BUILDER