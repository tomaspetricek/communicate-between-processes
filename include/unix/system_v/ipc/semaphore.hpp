#ifndef UNIX_SYSTEM_V_IPC_SEMAPHORE_HPP
#define UNIX_SYSTEM_V_IPC_SEMAPHORE_HPP

#include <cassert>
#include <expected>
#include <sys/sem.h>

#include "unix/system_v/ipc/primitive.hpp"

namespace unix::system_v::ipc
{
    class semaphore : public ipc::primitive
    {
    };
}

#endif // UNIX_SYSTEM_V_IPC_SEMAPHORE_HPP