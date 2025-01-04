#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <expected>
#include <optional>
#include <print>
#include <span>
#include <string_view>
#include <thread>
#include <utility>

#include "lock_free/message_ring_buffer.hpp"
#include "unix/error_code.hpp"
#include "unix/ipc/system_v/group_notifier.hpp"
#include "unix/ipc/system_v/semaphore_set.hpp"
#include "unix/permissions_builder.hpp"
#include "unix/resource_remover.hpp"

using namespace std::literals::chrono_literals;

namespace async
{
    template <std::size_t MessageQueueCapacity>
    class writer_group
    {
        using message_queue_type =
            lock_free::message_ring_buffer<MessageQueueCapacity>;

        std::atomic<bool> stop_flag{false};
        message_queue_type &message_queue_;
        const unix::ipc::system_v::group_notifier &read_message_bytes_notifier_;
        const unix::ipc::system_v::group_notifier &written_message_count_notifier_;

    public:
        explicit writer_group(
            lock_free::message_ring_buffer<MessageQueueCapacity> &message_queue,
            const unix::ipc::system_v::group_notifier &read_message_bytes_notifier,
            const unix::ipc::system_v::group_notifier
                &written_message_count_notifier) noexcept
            : message_queue_{message_queue},
              read_message_bytes_notifier_{read_message_bytes_notifier},
              written_message_count_notifier_{written_message_count_notifier} {}

        void start(std::size_t writer_id) noexcept
        {
            std::vector<char> message;

            while (true)
            {
                const auto message_written =
                    written_message_count_notifier_.wait_for_one();

                if (!message_written)
                {
                    std::println("failed waiting for message being written due to: {}",
                                 unix::to_string(message_written.error()));
                    return;
                }
                if (stop_flag.load(std::memory_order_acquire))
                {
                    return;
                }
                // ToDo: check the failure reason
                while (!message_queue_.try_pop(message))
                {
                }
                const auto read_bytes =
                    message_queue_type::required_message_storage(message.size());
                const auto read_message = read_message_bytes_notifier_.notify(read_bytes);

                if (!read_message)
                {
                    std::println("failed notifying about message being read due to: {}",
                                 unix::to_string(read_message.error()));
                    return;
                }
                std::println("id: {}, msg: {}", writer_id,
                             std::string_view{message.data(), message.size()});
            }
        }

        void stop() noexcept { stop_flag.store(true, std::memory_order_release); }
    };

    template <class First, class... Rest>
    constexpr std::size_t size_of() noexcept
    {
        return (sizeof(First) + ... + sizeof(Rest));
    }

    template <class... Args>
    static void format_string(const char *, const Args &...args) {}

    template <std::size_t Capacity>
    class message_buffer
    {
        using data_type = char;
        std::size_t size_{0};
        std::array<data_type, Capacity> data_ = {};

    public:
        void append(std::span<const char> values) noexcept
        {
            assert((size_ + values.size()) <= Capacity);

            for (std::size_t index{0};
                 index < std::min(values.size(), data_.size() - size_); ++index)
            {
                data_[size_++] = values[index];
            }
        }

        std::size_t size() const noexcept { return size_; }

        std::span<const char> span() const noexcept
        {
            return std::span<const char>{data_.data(), size_};
        }
    };

    template <std::size_t WriterCount, std::size_t MessageQueueCapacity>
    class logger
    {
        using writers_type = writer_group<MessageQueueCapacity>;
        using message_queue_type =
            lock_free::message_ring_buffer<MessageQueueCapacity>;

        std::array<std::thread, WriterCount> threads_;
        lock_free::message_ring_buffer<MessageQueueCapacity> message_queue_;
        unix::ipc::system_v::semaphore_set semaphores_;
        unix::ipc::system_v::group_notifier read_message_bytes_notifier_;
        unix::ipc::system_v::group_notifier written_message_count_notifier_;
        writers_type writers_;

        static constexpr std::size_t sem_count{2};
        static constexpr std::size_t read_message_bytes_sem_index{0};
        static constexpr std::size_t written_message_sem_index{1};

    public:
        explicit logger(
            const unix::ipc::system_v::semaphore_set &semaphores) noexcept
            : semaphores_{std::move(semaphores)},
              read_message_bytes_notifier_{semaphores_, read_message_bytes_sem_index, MessageQueueCapacity},
              written_message_count_notifier_{semaphores_, written_message_sem_index, WriterCount},
              writers_{message_queue_, read_message_bytes_notifier_,
                       written_message_count_notifier_}
        {
            for (std::size_t index{0}; index < threads_.size(); ++index)
            {
                threads_[index] =
                    std::move(std::thread{&writers_type::start, &writers_, index});
            }
        }

        logger(const logger &other) = delete;
        logger &operator=(const logger &other) = delete;

        logger(logger &&other) noexcept = delete;
        logger &operator=(logger &&other) noexcept = delete;

        static std::optional<logger> create() noexcept
        {
            constexpr auto perms = unix::permissions_builder{}
                                       .owner_can_read()
                                       .owner_can_write()
                                       .owner_can_execute()
                                       .group_can_read()
                                       .group_can_write()
                                       .group_can_execute()
                                       .others_can_read()
                                       .others_can_write()
                                       .others_can_execute()
                                       .get();
            auto semaphores_created =
                unix::ipc::system_v::semaphore_set::create_private(sem_count, perms);

            if (!semaphores_created)
            {
                std::println("failed to create semaphores due to: {}",
                             unix::to_string(semaphores_created.error()).data());
                return std::nullopt;
            }
            auto &semaphores = semaphores_created.value();

            std::array<unsigned short, sem_count> init_values{
                {MessageQueueCapacity, 0}};
            const auto all_initialized = semaphores.set_values(init_values);

            if (!all_initialized)
            {
                std::println("failed to initialize semaphore set valued: {}",
                             unix::to_string(all_initialized.error()).data());
                return std::nullopt;
            }
            std::println("all semaphores from the set initialized");
            return std::optional<logger>{std::in_place, semaphores};
        }

        ~logger() noexcept
        {
            written_message_count_notifier_.notify_all();
            writers_.stop();

            for (auto &thread : threads_)
            {
                thread.join();
            }
            assert(semaphores_.remove());
        }

        template <std::size_t Size, class... Args>
        bool log(const char (&format)[Size], const Args &...args) noexcept
        {
            constexpr std::size_t message_size = Size;
            message_buffer<message_size> message;
            message.append(format);

            const auto read_bytes =
                message_queue_type::required_message_storage(message.size());
            const auto message_read = read_message_bytes_notifier_.wait_for(read_bytes);

            if (!message_read)
            {
                std::println("failed waiting for message bytes being read due to: {}",
                             unix::to_string(message_read.error()));
                return false;
            }
            // ToDo: check the failure reason
            while (!message_queue_.try_push(message.span()))
            {
            }
            const auto message_written = written_message_count_notifier_.notify_one();

            if (!message_written)
            {
                std::println("failed notifying about message being written due to: {}",
                             unix::to_string(message_written.error()));
                return false;
            }
            return true;
        }
    };
} // namespace async

int main(int, char **)
{
    constexpr std::size_t writer_count{4}, message_queue_capacity{1'024},
        message_count{100}, sem_count{2}, read_message_bytes_sem_index{0},
        written_message_sem_index{1};
    auto logger_created =
        async::logger<writer_count, message_queue_capacity>::create();

    if (!logger_created)
    {
        std::println("failed to create async logger");
        return EXIT_FAILURE;
    }
    auto &logger = logger_created.value();

    for (std::size_t index{0}; index < message_count; ++index)
    {
        logger.log("hello {}", 42, 24.5);
        std::this_thread::sleep_for(5ms);
    }
}