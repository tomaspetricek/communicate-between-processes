#include <print>
#include <thread>

class background_task
{
    void do_something() const
    {
    }
    void do_something_else() const {}

public:
    void operator()() const
    {
        do_something();
        do_something_else();
        std::println("performing some background task");
    }
};

void do_something_in_current_thread() {}

class thread_guard
{
    std::thread &thread_;

public:
    explicit thread_guard(std::thread &thread) : thread_(thread) {}

    ~thread_guard()
    {
        if (thread_.joinable())
        {
            thread_.join();
        }
    }
    // forbid copying
    thread_guard(const thread_guard &) = delete;
    thread_guard operator=(const thread_guard &) = delete;
};

int main()
{
    std::thread task([]()
                     { std::println("threading"); }); // initial function
    background_task f;
    std::thread other_task(f);

    thread_guard guard1(task);
    thread_guard guard2(other_task);
}