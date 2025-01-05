#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <expected>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <optional>
#include <print>
#include <span>
#include <string_view>
#include <thread>
#include <tuple>
#include <utility>

#include "core/error_code.hpp"
#include "lock_free/message_ring_buffer.hpp"
#include "unix/error_code.hpp"
#include "unix/ipc/system_v/group_notifier.hpp"
#include "unix/ipc/system_v/semaphore_set.hpp"
#include "unix/permissions_builder.hpp"
#include "unix/resource_remover.hpp"

using namespace std::literals::chrono_literals;

namespace async
{
    class output_message_buffer
    {
        std::vector<lock_free::message_t> buffer_;
        std::size_t read_count_{0};

    public:
        template <class Value>
        Value read() noexcept
        {
            assert((buffer_.size() - read_count_) >= sizeof(Value));
            const void *data =
                reinterpret_cast<std::byte *>(buffer_.data()) + read_count_;
            const auto value = *reinterpret_cast<const std::size_t *>(data);
            read_count_ += sizeof(Value);
            return value;
        }

        template <class Value>
        std::span<Value> read(std::size_t size) noexcept
        {
            const auto total_size = sizeof(Value) * size;
            assert((buffer_.size() - read_count_) >= total_size);
            void *data = reinterpret_cast<std::byte *>(buffer_.data()) + read_count_;
            const auto array = std::span<Value>{reinterpret_cast<Value *>(data), size};
            read_count_ += total_size;
            return array;
        }

        std::size_t size() const noexcept { return buffer_.size(); }

        bool all_read() const noexcept { return read_count_ == buffer_.size(); }

        std::size_t remaining() const noexcept
        {
            assert(read_count_ < buffer_.size());
            return buffer_.size() - read_count_;
        }

        void reset() noexcept { read_count_ = 0; }

        std::vector<lock_free::message_t> &buffer() noexcept { return buffer_; }
    };

    template <std::size_t MessageQueueCapacity>
    class writer_group
    {
        using message_queue_type =
            lock_free::message_ring_buffer<MessageQueueCapacity>;

        std::atomic<bool> stop_flag{false};
        message_queue_type &message_queue_;
        unix::ipc::system_v::group_notifier read_message_bytes_notifier_;
        unix::ipc::system_v::group_notifier written_message_count_notifier_;
        std::atomic<std::int32_t> retry_pop_count_{0};
        std::atomic<std::int32_t> pop_count_{0};

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
            output_message_buffer message;

            while (true)
            {
                std::println("[{}] waiting for message being written", writer_id);
                const auto message_written =
                    written_message_count_notifier_.wait_for_one();

                if (!message_written)
                {
                    std::println("[{}] failed waiting for message being written due to: {}",
                                 writer_id, unix::to_string(message_written.error()));
                    return;
                }
                std::println("[{}] message written", writer_id);

                if (stop_flag.load(std::memory_order_acquire))
                {
                    std::println("[{}] stop flag received", writer_id);
                    return;
                }
                message.reset();
                bool popped{false};

                while (!popped)
                {
                    std::println("[{}] popping out message", writer_id);
                    const auto message_popped = message_queue_.try_pop(message.buffer());

                    if (message_popped)
                    {
                        popped = true;
                        continue;
                    }
                    const auto error = message_popped.error();

                    if (error != core::error_code::again)
                    {
                        std::println("[{}] failed to pop message due to: {}", writer_id,
                                     core::to_string(error).data());
                        return;
                    }
                    std::println("[{}] failed to pop message, trying again", writer_id);
                    retry_pop_count_.fetch_add(1, std::memory_order_relaxed);
                }
                pop_count_.fetch_add(1, std::memory_order_relaxed);

                std::println("[{}] notifying about message being read", writer_id);
                const auto read_bytes =
                    message_queue_type::required_message_storage(message.buffer().size());
                const auto read_message = read_message_bytes_notifier_.notify(read_bytes);

                if (!read_message)
                {
                    std::println(
                        "[{}] failed notifying about message being read due to: {}",
                        writer_id, unix::to_string(read_message.error()));
                    return;
                }
                std::println("[{}] formatting log", writer_id);
                std::this_thread::sleep_for(3ms);

                const auto format_func_addr = message.read<uintptr_t>();
                void (*format_func)(output_message_buffer &) =
                    reinterpret_cast<void (*)(output_message_buffer &)>(format_func_addr);
                format_func(message);
            }
        }

        void stop() noexcept { stop_flag.store(true, std::memory_order_release); }

        std::size_t retry_pop_count() const noexcept
        {
            return retry_pop_count_.load(std::memory_order_relaxed);
        }

        std::size_t popped_count() const noexcept
        {
            return pop_count_.load(std::memory_order_relaxed);
        }
    };

    template <class First, class... Rest>
    constexpr std::size_t size_of() noexcept
    {
        return (sizeof(First) + ... + sizeof(Rest));
    }

    template <std::size_t Capacity>
    class input_message_buffer
    {
        std::size_t size_{0};
        std::array<lock_free::message_t, Capacity> buffer_ = {};

    public:
        template <class Value>
        void write(const std::span<const Value> &values) noexcept
        {
            const auto total_size = sizeof(Value) * values.size();
            assert((size_ + total_size) <= Capacity);
            const auto min_size = std::min(total_size, buffer_.size() - size_);
            std::byte *buff = reinterpret_cast<std::byte *>(buffer_.data());
            std::memcpy(buff + size_, values.data(), min_size);
            size_ += min_size;
        }

        template <class Value>
        void write(const Value &value) noexcept
        {
            const auto size = sizeof(Value);
            assert((size_ + size) <= Capacity);
            const auto min_size = std::min(size, buffer_.size() - size_);
            std::byte *buff = reinterpret_cast<std::byte *>(buffer_.data());
            std::memcpy(buff + size_, &value, min_size);
            size_ += min_size;
        }

        std::size_t size() const noexcept { return size_; }

        std::span<const lock_free::message_t> span() const noexcept
        {
            return std::span<const lock_free::message_t>{buffer_.data(), size_};
        }

        bool full() const noexcept { return size_ == Capacity; }
    };

    template <std::size_t Index = 0, class... Values>
    void read_values(output_message_buffer &buffer,
                     std::tuple<Values...> &values) noexcept
    {
        if constexpr (Index < sizeof...(Values))
        {
            std::get<Index>(values) = buffer.read<
                typename std::tuple_element<Index, std::tuple<Values...>>::type>();
            return read_values<Index + 1, Values...>(buffer, values);
        }
    }

    template <class... Values>
    static void format_message(output_message_buffer &message) noexcept
    {
        std::tuple<Values...> values;
        read_values(message, values);
        const auto format = message.read<char>(message.remaining());
        std::apply([&format](auto &&...vals)
                   { std::printf(format.data(), vals...); },
                   values);
    }

    template <std::size_t Index, std::size_t Count, class Last>
    void fill_buffer(input_message_buffer<Count> &buffer, Last &&last) noexcept
    {
        buffer.write(last);
        assert(buffer.full());
    }

    template <std::size_t Index = 0, std::size_t Count, class First, class... Rest>
    void fill_buffer(input_message_buffer<Count> &buffer, First &&first,
                     Rest &&...rest) noexcept
    {
        buffer.write(first);
        return fill_buffer<Index + 1, Count, Rest...>(buffer,
                                                      std::forward<Rest>(rest)...);
    }

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
        std::int32_t retry_push_count_{0};
        std::int32_t pushed_count_{0};

        static constexpr std::size_t sem_count{2};
        static constexpr std::size_t read_message_bytes_sem_index{0};
        static constexpr std::size_t written_message_sem_index{1};

    public:
        explicit logger(const unix::ipc::system_v::semaphore_set &semaphores) noexcept
            : semaphores_{std::move(semaphores)},
              read_message_bytes_notifier_{semaphores_, read_message_bytes_sem_index,
                                           MessageQueueCapacity},
              written_message_count_notifier_{semaphores_, written_message_sem_index,
                                              WriterCount},
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
        bool log(const char (&format)[Size], Args &&...args) noexcept
        {
            static_assert(Size > 0);
            const auto format_size = Size - 1;
            constexpr std::size_t message_size =
                sizeof(uintptr_t >) + size_of<Args...>() + (sizeof(char) * format_size);
            input_message_buffer<message_size> message;
            fill_buffer(message, &format_message<Args...>, std::forward<Args>(args)...,
                        std::span<const char>{format, format_size});
            assert(message_size == message.size());

            const auto read_bytes =
                message_queue_type::required_message_storage(message.size());
            const auto message_read = read_message_bytes_notifier_.wait_for(read_bytes);

            if (!message_read)
            {
                std::println("failed waiting for message bytes being read due to: {}",
                             unix::to_string(message_read.error()));
                return false;
            }
            bool pushed{false};

            while (!pushed)
            {
                const auto message_pushed = message_queue_.try_push(message.span());

                if (message_pushed)
                {
                    pushed = true;
                    continue;
                }
                const auto &error = message_pushed.error();

                if (error != core::error_code::again)
                {
                    std::println("failed to push message due to {}",
                                 core::to_string(error).data());
                    return false;
                }
                std::println("failed to push message, trying again");
                retry_push_count_++;
            }
            pushed_count_++;
            const auto message_written = written_message_count_notifier_.notify_one();

            if (!message_written)
            {
                std::println("failed notifying about message being written due to: {}",
                             unix::to_string(message_written.error()));
                return false;
            }
            return true;
        }

        bool wait_till_all_popped() noexcept
        {
            const auto all_logged = read_message_bytes_notifier_.wait_for_all();

            if (!all_logged)
            {
                std::println("failed waiting for all messages being read due to: {}",
                             unix::to_string(all_logged.error()));
                return false;
            }
            return true;
        }

        std::size_t retry_push_count() const noexcept { return retry_push_count_; }

        std::size_t retry_pop_count() const noexcept
        {
            return writers_.retry_pop_count();
        }

        std::size_t pushed_count() const noexcept { return pushed_count_; }

        std::size_t popped_count() const noexcept { return writers_.popped_count(); }
    };
} // namespace async

int main(int, char **)
{
    constexpr std::size_t capacity = sizeof(std::int32_t) + sizeof(float);
    async::input_message_buffer<capacity> buffer;
    async::fill_buffer(buffer, std::int32_t{10}, float{6.7});

    const auto output_file_path =
        std::string_view{"/Users/tomaspetricek/Documents/repos/"
                         "communicate-between-processes/logs/async_logger.log"};
    FILE *file = freopen(output_file_path.data(), "w", stdout);

    if (!file)
    {
        std::cerr << "failed redirecting cout to a file" << std::endl;
        return EXIT_FAILURE;
    }
    // redirect to file
    {
        constexpr std::size_t writer_count{7}, message_queue_capacity{2'048},
            message_count{1'000}, sem_count{2}, read_message_bytes_sem_index{0},
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
            const auto logged = logger.log(
                "The quick brown fox jumps over the lazy dog while enjoying a "
                "sunny day in the park, watching the birds soar across the sky. %d, "
                "%d\n",
                std::int32_t{42}, std::int32_t{24});

            if (!logged)
            {
                std::println("failed to make a log");
                return EXIT_FAILURE;
            }
            std::this_thread::sleep_for(std::chrono::nanoseconds{16});
        }
        std::println("message generation done");

        if (!logger.wait_till_all_popped())
        {
            return EXIT_FAILURE;
        }
        std::println("retry push count: {}", logger.retry_push_count());
        std::println("retry pop count: {}", logger.retry_pop_count());
        std::println("pushed count: {}", logger.pushed_count());
        std::println("popped count: {}", logger.popped_count());
    }
    // restore stdout to console
    std::fflush(stdout);
    std::freopen("/dev/tty", "w", stdout);
    return EXIT_SUCCESS;
}