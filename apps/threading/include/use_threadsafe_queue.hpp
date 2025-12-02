#ifndef THREADING_USE_THREADSAFE_QUEUE_HPP
#define THREADING_USE_THREADSAFE_QUEUE_HPP

#include "threadsafe_queue.hpp"

namespace v2
{
    struct data_chunk
    {
    };

    threadsafe_queue<data_chunk> data_queue;

    bool more_data_to_prepare()
    {
        return true;
    }

    data_chunk prepare_data()
    {
        return data_chunk();
    }

    void process_data(const data_chunk &data)
    {
    }

    bool is_last_chunk() { return false; }

    void data_preparation_thread()
    {
        while (more_data_to_prepare())
        {
            const auto data = prepare_data();
            data_queue.push(data);
        }
    }

    void data_processing_thread()
    {
        bool keep_processing = true;

        while (keep_processing)
        {
            data_chunk data;
            data_queue.wait_and_pop(data);
            process_data(data);
            keep_processing = !is_last_chunk();
        }
    }
}

#endif // THREADING_USE_THREADSAFE_QUEUE_HPP