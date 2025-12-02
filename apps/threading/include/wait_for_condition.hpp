#ifndef THREADING_WAIT_FOR_CONDITION_HPP
#define THREADING_WAIT_FOR_CONDITION_HPP

#include <condition_variable>
#include <mutex>
#include <queue>

struct data_chunk
{
};

std::mutex mutex;
std::queue<data_chunk> data_queue;
std::condition_variable data_prepared;

bool more_data_to_prepare() { return true; }

data_chunk prepare_data() { return data_chunk(); }

bool is_last_chunk() { return false; }

void process_data(const data_chunk &) {}

void data_preparation_thread()
{
    while (more_data_to_prepare())
    {
        const auto data = prepare_data();
        {
            std::lock_guard lock(mutex);
            data_queue.push(data);
        }
        data_prepared.notify_one();
    }
}

void data_processing_thread()
{
    bool keep_processing = true;

    while (keep_processing)
    {
        std::unique_lock lock(mutex);
        data_prepared.wait(lock, []
                           { return !data_queue.empty(); });
        const auto data = data_queue.front();
        data_queue.pop();
        lock.unlock();
        process_data(data);
        keep_processing = is_last_chunk();
    }
}

#endif // THREADING_WAIT_FOR_CONDITION_HPP