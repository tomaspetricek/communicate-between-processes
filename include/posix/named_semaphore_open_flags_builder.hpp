#ifndef POSIX_NAMED_SEMAPHORE_OPEN_FLAGS_BUILDER
#define POSIX_NAMED_SEMAPHORE_OPEN_FLAGS_BUILDER

#include "posix/open_flags.hpp"
#include "posix/open_flags_builder.hpp"


namespace posix
{
    class named_semaphore_open_flags_builder : public open_flags_builder<named_semaphore_open_flags_builder>
    {
    public:
        explicit named_semaphore_open_flags_builder() noexcept = default;
    };
} // namespace posix

#endif // POSIX_NAMED_SEMAPHORE_OPEN_FLAGS_BUILDER