#ifndef UNIX_POSIX_IPC_MESSAGE_QUEUE_HPP
#define UNIX_POSIX_IPC_MESSAGE_QUEUE_HPP

#include <cassert>
#include <expected>
#include <fcntl.h>
// #include <mqueue.h> // not available on OSX
#include <string>

#include "error_code.hpp"
#include "utility.hpp"
#include "unix/posix/open_flags.hpp"
#include "posix/ipc/primitive.hpp"


namespace unix::posix::ipc
{
    class message_queue : public ipc::primitive
    {
    public:
        explicit message_queue() noexcept = default;

        static std::expected<posix::message_queue, error_code>
        create(std::string name, open_flags_t flags) noexcept
        {
            assert(is_valid_pathname(name));
            // mq_open(...)
            return std::expected<posix::message_queue, error_code>{std::in_place};
        }

        std::expected<void, error_code> close() noexcept
        {
            // mq_close(...);
            return std::expected<void, error_code>{};
        }

        std::expected<void, error_code> unlink() noexcept
        {
            // mq_unlink();
            return std::expected<void, error_code>{};
        }

        std::expected<void, error_code> send(/*...*/) noexcept
        {
            // mq_send(...)
            return std::expected<void, error_code>{};
        }

        std::expected<void, error_code> receive(/*...*/) noexcept
        {
            // mq_receive(/...)
            return std::expected<void, error_code>{};
        }

        std::expected<void, error_code> notify(/*...*/) noexcept
        {
            // mq_notify(...)
            return std::expected<void, error_code>{};
        }
    };
} // namespace unix::posix::ipc

#endif // UNIX_POSIX_IPC_MESSAGE_QUEUE_HPP
