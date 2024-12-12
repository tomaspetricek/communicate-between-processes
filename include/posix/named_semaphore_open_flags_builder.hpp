#ifndef POSIX_NAMED_SEMAPHORE_OPEN_FLAGS_BUILDER
#define POSIX_NAMED_SEMAPHORE_OPEN_FLAGS_BUILDER

#include "posix/open_flags.hpp"

namespace posix
{
    class named_semaphore_open_flags_builder
    {
        open_flags_t flags_{no_open_flags};

    public:
        named_semaphore_open_flags_builder &create_from_opened() noexcept
        {
            // default creation
            return *this;
        };

        named_semaphore_open_flags_builder &create() noexcept
        {
            flags_ |= O_CREAT;
            return *this;
        }

        named_semaphore_open_flags_builder &create_exclusively() noexcept
        {
            flags_ |= O_CREAT | O_EXCL;
            return *this;
        }

        open_flags_t get() const noexcept { return flags_; }
    };
} // namespace posix

#endif // POSIX_NAMED_SEMAPHORE_OPEN_FLAGS_BUILDER