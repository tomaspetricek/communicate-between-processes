#ifndef POSIX_MESSAGE_QUEUE_OPEN_FLAGS_BUILDER
#define POSIX_MESSAGE_QUEUE_OPEN_FLAGS_BUILDER

#include <array>
#include <cassert>

#include "posix/open_flags.hpp"

namespace posix
{
    enum class access_mode
    {
        read_only,  // O_RDONLY
        write_only, // O_WRONLY
        read_write, // O_RDWR
    };
    
    class message_queue_open_flags_builder
    {
        open_flags_t flags_{no_open_flags};

    public:
        explicit message_queue_open_flags_builder(access_mode access) noexcept
        {
            constexpr std::size_t access_mode_count = 3;
            const auto access_index = static_cast<std::size_t>(access);
            assert(access_index >= std::size_t{0});
            assert(access_index < access_mode_count);
            const auto access_flags = std::array<open_flags_t, access_mode_count>{
                O_RDONLY, O_WRONLY, O_RDWR};
            flags_ |= access_flags[access_index];
        }

        message_queue_open_flags_builder &create_from_opened() noexcept
        {
            // default creation
            return *this;
        };

        message_queue_open_flags_builder &create() noexcept
        {
            flags_ |= O_CREAT;
            return *this;
        }

        message_queue_open_flags_builder &create_exclusively() noexcept
        {
            flags_ |= O_CREAT | O_EXCL;
            return *this;
        }

        message_queue_open_flags_builder &is_blocking() noexcept
        {
            // default mode
            return *this;
        }

        message_queue_open_flags_builder &is_non_blocking() noexcept
        {
            flags_ |= O_NONBLOCK;
            return *this;
        }

        open_flags_t get() const noexcept
        {
            return flags_;
        }
    };
} // namespace posix

#endif // POSIX_MESSAGE_QUEUE_OPEN_FLAGS_BUILDER