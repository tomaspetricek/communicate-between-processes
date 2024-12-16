#include <cassert>

#include "unix/posix/ipc/message_queue_open_flags_builder.hpp"

int main(int, char **)
{
    using namespace unix::posix;

    constexpr auto flags =
        ipc::message_queue_open_flags_builder{ipc::access_mode::read_write}
            .create_exclusively()
            .is_non_blocking()
            .get();
    static_assert((O_RDWR | O_CREAT | O_EXCL | O_NONBLOCK) == flags);
}