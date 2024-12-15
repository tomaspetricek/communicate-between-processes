#ifndef UNIX_POSIX_IPC_NAMED_SEMAPHORE_OPEN_FLAGS_BUILDER
#define UNIX_POSIX_IPC_NAMED_SEMAPHORE_OPEN_FLAGS_BUILDER

#include "unix/posix/ipc/open_flags.hpp"
#include "unix/posix/ipc/open_flags_builder.hpp"


namespace unix::posix::ipc
{
    class named_semaphore_open_flags_builder : public open_flags_builder<named_semaphore_open_flags_builder>
    {
    public:
        constexpr explicit named_semaphore_open_flags_builder() noexcept = default;
    };
} // namespace unix::posix::ipc

#endif // UNIX_POSIX_IPC_NAMED_SEMAPHORE_OPEN_FLAGS_BUILDER