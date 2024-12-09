#include <print>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>

namespace
{
    constexpr std::size_t buffer_size{5};
    std::vector<int> buffer;

    // semaphores
    sem_t empty_slots;            // tracks available slots in the buffer
    sem_t filled_slots;           // tracks the number of items in the buffer
    pthread_mutex_t buffer_mutex; // ensures mutual exclusion for buffer access

    void *producer(void *arg)
    {
        int producer_id = *(int *)arg;

        for (int i{0}; i < 10; ++i)
        {
            int item = rand() % 100;

            sem_wait(&empty_slots);            // wait if buffer is full
            pthread_mutex_lock(&buffer_mutex); // ensure exclusive access to the buffer

            // critical section
            buffer.push_back(item);
            std::println("producer: {} produced: {}", producer_id, item);

            pthread_mutex_unlock(&buffer_mutex); // release buffer lock
            sem_post(&filled_slots);             // signal that a new item is available

            sleep(1); // simulate production time
        }
        pthread_exit(nullptr);
    }

    void *consumer(void *arg)
    {
        int consumer_id = *(int *)arg;

        for (int i{0}; i < 10; ++i)
        {
            sem_wait(&filled_slots);           // wait if buffer is empty
            pthread_mutex_lock(&buffer_mutex); // ensure exclusive access to the buffer

            // critical section
            int item = buffer.back();
            buffer.pop_back();
            std::println("consumer: {} consumed: {}", consumer_id, item);

            pthread_mutex_unlock(&buffer_mutex); // release buffer lock
            sem_post(&empty_slots);              // signal that a slot is free

            sleep(2); // simulate consumption time
        }
        pthread_exit(nullptr);
    }
}

int main(int, char **)
{
    // initilaize semaphores and mutex
    sem_init(&empty_slots, 0, buffer_size); // buffer size empty slots initially
    sem_init(&filled_slots, 0, 0);          // no items in the biffer initially
    pthread_mutex_init(&buffer_mutex, nullptr);

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

    // clean up
    sem_destroy(&empty_slots);
    sem_destroy(&filled_slots);
    pthread_mutex_destroy(&buffer_mutex);
    return EXIT_SUCCESS;
}