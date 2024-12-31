#ifndef BUFFERING_OCCUPATION_PRODUCER_HPP
#define BUFFERING_OCCUPATION_PRODUCER_HPP

#include <atomic>
#include <functional>

#include "unix/ipc/system_v/group_notifier.hpp"
#include "unix/process.hpp"

#include "buffering/process_info.hpp"
#include "buffering/message_queue.hpp"

namespace buffering::occupation
{
    bool notify_about_production_completion(
        const unix::ipc::system_v::group_notifier &producers_notifier) noexcept
    {
        const auto production_done = producers_notifier.notify_one();

        if (!production_done)
        {
            std::println("failed to send signal about production being done");
            return false;
        }
        return true;
    }

    bool produce_messages(
        const process_info &info, const std::size_t message_count,
        const unix::ipc::system_v::group_notifier &message_read_notifier,
        const unix::ipc::system_v::group_notifier &message_written_notifier,
        message_queue_t &message_queue,
        std::atomic<std::int32_t> &produced_message_count) noexcept
    {
        message_t message;

        for (std::size_t i{0}; i < message_count; ++i)
        {
            const auto ret =
                snprintf(message.data(), message.size(),
                         "message with id: %zu from producer: %zu", i, info.group_id);

            if (unix::operation_failed(ret))
            {
                std::println("failed to create message");
                return false;
            }
            message_queue.push(message);
            produced_message_count++;

            std::println("message written into shared memeory by producer: {}",
                         info.group_id);
            const auto message_written = message_written_notifier.notify_one();

            if (!message_written)
            {
                std::println(
                    "failed to send signal about message being written due to: {}",
                    unix::to_string(message_written.error()).data());
                return false;
            }
            const auto message_read = message_read_notifier.wait_for_one();

            if (!message_read)
            {
                std::println(
                    "failed to receive signal about message being read due to: {}",
                    unix::to_string(message_read.error()).data());
                return false;
            }
        }
        return true;
    }

    bool wait_till_production_start(
        const unix::ipc::system_v::group_notifier &producers_notifier) noexcept
    {
        const auto start_production = producers_notifier.wait_for_one();

        if (!start_production)
        {
            std::println("failed to receive starting signal from the parent");
            return false;
        }
        return true;
    }

    bool run_producer(
        const process_info &info, std::size_t message_count,
        const unix::ipc::system_v::group_notifier &producers_notifier,
        const unix::ipc::system_v::group_notifier &message_read_notifier,
        const unix::ipc::system_v::group_notifier &message_written_notifier,
        message_queue_t &message_queue,
        std::atomic<std::int32_t> &produced_message_count) noexcept
    {
        std::println("wait till production start");

        if (!wait_till_production_start(producers_notifier))
        {
            return EXIT_FAILURE;
        }
        std::println("production started");
        std::println("produce messages");

        if (!produce_messages(info, message_count, message_read_notifier,
                              message_written_notifier, message_queue,
                              produced_message_count))
        {
            return false;
        }
        std::println("message production done");
        std::println("notify about production completion");

        if (!notify_about_production_completion(producers_notifier))
        {
            return false;
        }
        std::println("notified about production completion");
        return true;
    }

    class producer
    {
    public:
        explicit producer(
            const process_info &info, std::size_t message_count,
            const unix::ipc::system_v::group_notifier &producers_notifier,
            const unix::ipc::system_v::group_notifier &message_read_notifier,
            const unix::ipc::system_v::group_notifier &message_written_notifier,
            message_queue_t &message_queue,
            std::atomic<std::int32_t> &produced_message_count) noexcept
            : info_{info}, message_count_{message_count},
              producers_notifier_{producers_notifier},
              message_read_notifier_{message_read_notifier},
              message_written_notifier_{message_written_notifier},
              message_queue_{message_queue},
              produced_message_count_{produced_message_count} {}

        bool run() noexcept
        {
            return run_producer(info_, message_count_, producers_notifier_,
                                message_read_notifier_, message_written_notifier_,
                                message_queue_, produced_message_count_);
        }

    private:
        process_info info_;
        std::size_t message_count_;
        std::reference_wrapper<const unix::ipc::system_v::group_notifier>
            producers_notifier_;
        std::reference_wrapper<const unix::ipc::system_v::group_notifier>
            message_read_notifier_;
        std::reference_wrapper<const unix::ipc::system_v::group_notifier>
            message_written_notifier_;
        std::reference_wrapper<message_queue_t> message_queue_;
        std::reference_wrapper<std::atomic<std::int32_t>> produced_message_count_;
    };
}

#endif // BUFFERING_OCCUPATION_PRODUCER_HPP