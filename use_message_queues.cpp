#include <cassert>

#include "posix/message_queue_open_flags_builder.hpp"

int main(int, char **)
{
    const auto flags =
        posix::message_queue_open_flags_builder{posix::access_mode::read_write}
            .create_only()
            .is_non_blocking()
            .get();
    assert((O_RDWR | O_CREAT | O_EXCL | O_NONBLOCK) == flags);
}