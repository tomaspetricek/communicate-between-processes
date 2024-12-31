#ifndef UNIX_IPC_SYSTEM_V_MESSAGE_QUEUE_HPP
#define UNIX_IPC_SYSTEM_V_MESSAGE_QUEUE_HPP

#include <cassert>
#include <expected>
#include <sys/msg.h>

#include "unix/ipc/system_v/primitive.hpp"


namespace unix::ipc::system_v
{
    class message_queue : public ipc::primitive {
    };
}

#endif // UNIX_IPC_SYSTEM_V_MESSAGE_QUEUE_HPP