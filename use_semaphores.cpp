#include <algorithm>
#include <cassert>
#include <iterator>
#include <print>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#include "unix/posix/mutex.hpp"
#include "unix/posix/ipc/named_semaphore.hpp"
#include "unix/posix/ipc/unnamed_semaphore.hpp"
#include "unix/permissions_builder.hpp"
#include "unix/posix/ipc/named_semaphore_open_flags_builder.hpp"

namespace
{
    constexpr std::size_t buffer_size{5};
    std::vector<int> buffer;

    // semaphores
    constexpr auto perms = unix::permissions_builder{}
                               .owner_can_read()
                               .owner_can_write()
                               .group_can_read()
                               .others_can_read()
                               .get();
    static_assert(0644 == perms);
    constexpr auto flags = unix::posix::ipc::named_semaphore_open_flags_builder{}
                               .create_exclusively()
                               .get();
    static_assert(flags == (O_CREAT | O_EXCL));
    auto empty_slots_created = unix::posix::ipc::named_semaphore::create("/empty", flags, perms, buffer_size); // all slots available in the beginning
    auto filled_slots_created = unix::posix::ipc::named_semaphore::create("/filled", flags, perms, 0);         // no slots are filled in the beginning
    auto &empty_slots = empty_slots_created.value();                                                           // tracks available slots in the buffer
    auto &filled_slots = filled_slots_created.value();                                                         // tracks the number of items in the buffer
    auto buffer_mutex_created = unix::posix::mutex::create();
    auto &buffer_mutex = buffer_mutex_created.value();

    void *producer(void *arg)
    {
        int producer_id = *(int *)arg;

        for (int i{0}; i < 10; ++i)
        {
            int item = rand() % 100;

            assert(empty_slots.wait());  // wait if buffer is full
            assert(buffer_mutex.lock()); // ensure exclusive access to the buffer

            // critical section
            buffer.push_back(item);
            std::println("producer: {} produced: {}", producer_id, item);

            assert(buffer_mutex.unlock()); // release buffer lock
            assert(filled_slots.post());   // signal that a new item is available

            sleep(1); // simulate production time
        }
        pthread_exit(nullptr);
    }

    void *consumer(void *arg)
    {
        int consumer_id = *(int *)arg;

        for (int i{0}; i < 10; ++i)
        {
            assert(filled_slots.wait()); // wait if buffer is empty
            assert(buffer_mutex.lock()); // ensure exclusive access to the buffer

            // critical section
            int item = buffer.back();
            buffer.pop_back();
            std::println("consumer: {} consumed: {}", consumer_id, item);

            assert(buffer_mutex.unlock()); // release buffer lock
            assert(empty_slots.post());    // signal that a slot is free

            sleep(2); // simulate consumption time
        }
        pthread_exit(nullptr);
    }
}

int main(int, char **)
{
    assert(buffer_mutex_created.has_value());
    assert(empty_slots_created.has_value());
    assert(filled_slots_created.has_value());

    buffer.reserve(buffer_size);

    // cretae producer and conumer threads
    constexpr std::size_t producer_count{2}, consumer_count{2};
    pthread_t producers[producer_count], consumers[consumer_count];
    int producer_ids[producer_count] = {1, 2};
    int consumer_ids[consumer_count];
    std::copy(std::begin(producer_ids), std::end(producer_ids), std::begin(consumer_ids));

    static_assert(producer_count == consumer_count);
    for (std::size_t i{0}; i < producer_count; ++i)
    {
        pthread_create(&producers[i], nullptr, producer, &producer_ids[i]);
        pthread_create(&consumers[i], nullptr, consumer, &consumer_ids[i]);
    }

    // wait for threads to finish
    for (std::size_t i{0}; i < producer_count; ++i)
    {
        pthread_join(producers[i], nullptr);
        pthread_join(consumers[i], nullptr);
    }
    empty_slots.unlink();
    filled_slots.unlink();

    using namespace unix::posix;

    const auto created = ipc::unnamed_semaphore::create(ipc::shared_between::threads, 1);
    assert(!created);
    std::println("failed to create unnamed semaphore due to: {}", unix::to_string(created.error()).data());

    auto mutex_created = mutex::create();
    assert(mutex_created.has_value());
    auto &mutex = mutex_created.value();
    assert(mutex.lock());
    assert(mutex.unlock());
    return EXIT_SUCCESS;
}