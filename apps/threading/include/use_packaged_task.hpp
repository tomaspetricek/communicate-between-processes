#ifndef THREADING_USE_PACKAGED_TASK_HPP
#define THREADING_USE_PACKAGED_TASK_HPP

#include <deque>
#include <future>
#include <joining_thread.hpp>
#include <mutex>
#include <thread>
#include <utility>

namespace task
{
    std::mutex mutex;
    std::deque<std::packaged_task<void()>> tasks;

    bool gui_shutdown_message_received() { return false; }

    bool get_and_process_gui_message() { return true; }

    void gui_thread()
    {
        while (!gui_shutdown_message_received())
        {
            get_and_process_gui_message();

            std::packaged_task<void()> task;
            {
                std::lock_guard lock(mutex);

                if (tasks.empty())
                {
                    continue;
                }
                task = std::move(tasks.front());
                tasks.pop_front();
            }
            // perform task
            task();
        }
    }

    template <class Func>
    std::future<void> post_task_for_gui_thread(Func func)
    {
        std::packaged_task<void()> task(func);
        auto result = task.get_future();
        std::lock_guard lock(mutex);
        tasks.push_back(std::move(task));
        return result;
    }
} // namespace task

void use_packed_task()
{
    joining_thread gui_background_thread(task::gui_thread);
}

#endif // THREADING_USE_PACKAGED_TASK_HPP