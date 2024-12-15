#ifndef UNIX_SYSTEM_V_IPC_SHARED_MEMORY_HPP
#define UNIX_SYSTEM_V_IPC_SHARED_MEMORY_HPP

#include <cassert>
#include <expected>
#include <sys/msg.h>

#include "unix/system_v/ipc/primitive.hpp"


namespace unix::system_v::ipc
{
    class shared_memory : public ipc::primitive {
    };
}

#endif // UNIX_SYSTEM_V_IPC_SHARED_MEMORY_HPP