#include <cassert>
#include <print>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>

#include "posix/mutex.hpp"
#include "posix/unnamed_semaphore.hpp"

namespace
{
    constexpr std::size_t buffer_size{5};
    std::vector<int> buffer;

    // semaphores
    auto empty_slots_created = posix::unnamed_semaphore::create(posix::shared_between::threads, buffer_size); // all slots available in the beginning
    auto filled_slots_created = posix::unnamed_semaphore::create(posix::shared_between::threads, 0);          // no slots are filled in the beginning
    auto &empty_slots = empty_slots_created.value();                                                          // tracks available slots in the buffer
    auto &filled_slots = filled_slots_created.value();                                                        // tracks the number of items in the buffer
    auto buffer_mutex_created = posix::mutex::create();
    auto &buffer_mutex = buffer_mutex_created.value();

    void *producer(void *arg)
    {
        int producer_id = *(int *)arg;

        for (int i{0}; i < 10; ++i)
        {
            int item = rand() % 100;

            assert(empty_slots.wait());          // wait if buffer is full
            assert(buffer_mutex.lock()); // ensure exclusive access to the buffer

            // critical section
            buffer.push_back(item);
            std::println("producer: {} produced: {}", producer_id, item);

            assert(buffer_mutex.unlock()); // release buffer lock
            assert(filled_slots.post());           // signal that a new item is available

            sleep(1); // simulate production time
        }
        pthread_exit(nullptr);
    }

    void *consumer(void *arg)
    {
        int consumer_id = *(int *)arg;

        for (int i{0}; i < 10; ++i)
        {
            assert(filled_slots.wait());         // wait if buffer is empty
            assert(buffer_mutex.lock()); // ensure exclusive access to the buffer

            // critical section
            int item = buffer.back();
            buffer.pop_back();
            std::println("consumer: {} consumed: {}", consumer_id, item);

            assert(buffer_mutex.unlock()); // release buffer lock
            assert(empty_slots.post());            // signal that a slot is free

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

    // cretae producer and conumer threads
    pthread_t producers[2], consumers[2];
    int producer_ids[2] = {1, 2};
    int consumer_ids[2] = {1, 2};

    for (int i = 0; i < 2; ++i)
    {
        pthread_create(&producers[i], nullptr, producer, &producer_ids[i]);
        pthread_create(&consumers[i], nullptr, consumer, &consumer_ids[i]);
    }

    // wait for threads to finish
    for (int i = 0; i < 2; ++i)
    {
        pthread_join(producers[i], nullptr);
        pthread_join(consumers[i], nullptr);
    }
    return EXIT_SUCCESS;
}