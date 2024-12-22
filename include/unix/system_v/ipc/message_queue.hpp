#ifndef UNIX_SYSTEM_V_IPC_MESSAGE_QUEUE_HPP
#define UNIX_SYSTEM_V_IPC_MESSAGE_QUEUE_HPP

#include <cassert>
#include <expected>
#include <sys/msg.h>

#include "unix/system_v/ipc/primitive.hpp"


namespace unix::system_v::ipc
{
    class message_queue : public ipc::primitive {
    };
}

#endif // UNIX_SYSTEM_V_IPC_MESSAGE_QUEUE_HPP