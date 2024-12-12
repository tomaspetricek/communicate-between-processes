#include <cassert>

#include "posix/message_queue.hpp"

int main(int, char **)
{
    posix::message_queue_open_flags flags{posix::access_mode::read_write, posix::creation_mode::exclusive_create, false};
    assert((O_RDWR | O_CREAT | O_EXCL | O_NONBLOCK) == flags.translate());
}