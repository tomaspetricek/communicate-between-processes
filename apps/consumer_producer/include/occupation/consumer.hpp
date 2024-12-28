#ifndef OCCUPATION_CONSUMER_HPP
#define OCCUPATION_CONSUMER_HPP

#include <atomic>
#include <functional>

#include "unix/system_v/ipc/group_notifier.hpp"
#include "unix/process.hpp"

#include "process_info.hpp"
#include "message_queue.hpp"

namespace occupation
{
    bool consume_messages(
        const process_info &info,
        const unix::system_v::ipc::group_notifier &message_written_notifier,
        const unix::system_v::ipc::group_notifier &message_read_notifier,
        message_queue_t &message_queue, const unix::process_id_t &process_id,
        const std::atomic<bool> &done_flag,
        std::atomic<std::int32_t> &consumed_message_count) noexcept
    {
        while (true)
        {
            std::println("wait for message");
            const auto message_written = message_written_notifier.wait_for_one();

            if (done_flag.load())
            {
                std::println("done flag received");
                break;
            }
            if (!message_written)
            {
                std::println(
                    "failed to receive signal about message being written due to: {}",
                    unix::to_string(message_written.error()).data());
                return false;
            }
            const auto message = message_queue.pop();
            consumed_message_count++;
            std::println("received message: {} by child with id: {}", message.data(),
                         process_id);
            const auto message_read = message_read_notifier.notify_one();

            if (!message_read)
            {
                std::println("failed to send signal about message being read due to: {}",
                             unix::to_string(message_read.error()).data());
                return false;
            }
        }
        return true;
    }

    bool run_consumer(
        const process_info &info, unix::process_id_t process_id,
        const unix::system_v::ipc::group_notifier &message_read_notifier,
        const unix::system_v::ipc::group_notifier &message_written_notifier,
        message_queue_t &message_queue, std::atomic<bool> &done_flag,
        std::atomic<std::int32_t> &consumed_message_count)
    {
        std::println("consume messages");

        if (!consume_messages(info, message_written_notifier, message_read_notifier,
                              message_queue, process_id, done_flag,
                              consumed_message_count))
        {
            return false;
        }
        std::println("message consumption done");
        return true;
    }

    class consumer
    {
    public:
        explicit consumer(
            const process_info &info, unix::process_id_t process_id,
            const unix::system_v::ipc::group_notifier &message_read_notifier,
            const unix::system_v::ipc::group_notifier &message_written_notifier,
            message_queue_t &message_queue, std::atomic<bool> &done_flag,
            std::atomic<std::int32_t> &consumed_message_count) noexcept
            : info_{info}, process_id_{process_id},
              message_read_notifier_{message_read_notifier},
              message_written_notifier_{message_written_notifier},
              message_queue_{message_queue}, done_flag_{done_flag},
              consumed_message_count_{consumed_message_count} {}

        bool run() noexcept
        {
            return run_consumer(info_, process_id_, message_read_notifier_,
                                message_written_notifier_, message_queue_, done_flag_,
                                consumed_message_count_);
        }

    private:
        process_info info_;
        unix::process_id_t process_id_;
        std::reference_wrapper<const unix::system_v::ipc::group_notifier>
            message_read_notifier_;
        std::reference_wrapper<const unix::system_v::ipc::group_notifier>
            message_written_notifier_;
        std::reference_wrapper<message_queue_t> message_queue_;
        std::reference_wrapper<std::atomic<bool>> done_flag_;
        std::reference_wrapper<std::atomic<std::int32_t>> consumed_message_count_;
    };
}

#endif // OCCUPATION_CONSUMER_HPP