#ifndef POSIX_IPC_NAMED_SEMAPHORE_OPEN_FLAGS_BUILDER
#define POSIX_IPC_NAMED_SEMAPHORE_OPEN_FLAGS_BUILDER

#include "posix/ipc/open_flags.hpp"
#include "posix/ipc/open_flags_builder.hpp"


namespace posix::ipc
{
    class named_semaphore_open_flags_builder : public open_flags_builder<named_semaphore_open_flags_builder>
    {
    public:
        constexpr explicit named_semaphore_open_flags_builder() noexcept = default;
    };
} // namespace posix::ipc

#endif // POSIX_IPC_NAMED_SEMAPHORE_OPEN_FLAGS_BUILDER