#ifndef POSIX_MESSAGE_QUEUE_HPP
#define POSIX_MESSAGE_QUEUE_HPP

#include <cassert>
#include <expected>
#include <fcntl.h>
// #include <mqueue.h> // not available on OSX
#include <string>

#include "posix/error_code.hpp"
#include "posix/utility.hpp"

namespace posix
{
    enum class access_mode
    {
        read_only,
        write_only,
        read_write,
    };

    enum class creation_mode
    {
        already_exists,
        create,           // if exists just open, otherwise create it
        exclusive_create, // create only if it does not exist
    };

    class open_flags
    {
        access_mode access_;
        creation_mode creation_;
        bool is_blocking_;

        int translate_access_mode() const noexcept
        {
            if (access_ == access_mode::read_only)
            {
                return O_RDONLY;
            }
            if (access_ == access_mode::write_only)
            {
                return O_WRONLY;
            }
            assert(access_ == access_mode::read_write);
            return O_RDWR;
        }

    public:
        explicit open_flags(access_mode access,
                            creation_mode creation = creation_mode::already_exists,
                            bool is_blocking = true) noexcept
            : access_{access}, creation_{creation}, is_blocking_{is_blocking} {}

        int translate() const noexcept
        {
            int flags = 0;
            flags |= translate_access_mode();

            if (creation_ == creation_mode::create)
            {
                flags |= O_CREAT;
            }
            if (creation_ == creation_mode::exclusive_create)
            {
                flags |= O_CREAT | O_EXCL;
            }
            if (is_blocking_ == false)
            {
                flags |= O_NONBLOCK;
            }
            return flags;
        }
    };

    class message_queue
    {
    public:
        explicit message_queue() noexcept = default;

        static std::expected<posix::message_queue, error_code>
        create(std::string name, const open_flags &flags) noexcept
        {
            assert(is_valid_ipc_name(name));
            const auto translate_flags = flags.translate();
            // mq_open(...)
            return std::expected<posix::message_queue, error_code>{std::in_place};
        }

        message_queue(const message_queue &other) = delete;
        message_queue &operator=(const message_queue &other) = delete;

        message_queue(message_queue &&other) noexcept = delete;
        message_queue &operator=(message_queue &&other) noexcept = delete;

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
} // namespace posix

#endif // POSIX_MESSAGE_QUEUE_HPP
