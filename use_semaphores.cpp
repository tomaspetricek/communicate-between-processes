#include <algorithm>
#include <cassert>
#include <iterator>
#include <print>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#include "unix/sync/posix/mutex.hpp"
#include "unix/ipc/posix/named_semaphore.hpp"
#include "unix/ipc/posix/unnamed_semaphore.hpp"
#include "unix/permissions_builder.hpp"


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
    const auto empty_slots_created = unix::ipc::posix::named_semaphore::create_exclusively("/empty", perms, buffer_size); // all slots available in the beginning
    const auto filled_slots_created = unix::ipc::posix::named_semaphore::create_exclusively("/filled", perms, 0);         // no slots are filled in the beginning
    const auto &empty_slots = empty_slots_created.value();                                                           // tracks available slots in the buffer
    const auto &filled_slots = filled_slots_created.value();                                                         // tracks the number of items in the buffer
    auto buffer_mutex_created = unix::sync::posix::mutex::create();
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

    using namespace unix::ipc;

    const auto created = posix::unnamed_semaphore::create(posix::shared_between::threads, 1);
    assert(!created);
    std::println("failed to create unnamed semaphore due to: {}", unix::to_string(created.error()).data());

    auto mutex_created = unix::sync::posix::mutex::create();
    assert(mutex_created.has_value());
    auto &mutex = mutex_created.value();
    assert(mutex.lock());
    assert(mutex.unlock());
    return EXIT_SUCCESS;
}