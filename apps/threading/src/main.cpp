#include <memory>
#include <print>
#include <thread>

class background_task
{
    void do_something() const {}
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

struct widget_id
{
};
struct widget_data
{
};

struct X
{
public:
    void do_lengthy_work() { std::println("calling a method"); }
};

void update_data_for_widget(widget_id id, widget_data &data) {}

struct big_object
{
};
void process_big_object(std::unique_ptr<big_object> obj)
{
    std::println("processing a big object");
}

class scoped_thread
{
    std::thread thread_;

public:
    explicit scoped_thread(std::thread thread) : thread_(std::move(thread))
    {
        if (!thread_.joinable())
        {
            throw std::logic_error("No thread");
        }
    }
    ~scoped_thread() { thread_.join(); }
    // forbid copying
    scoped_thread(const scoped_thread &) = delete;
    scoped_thread operator=(const scoped_thread &) = delete;
};

class joining_thread
{
    std::thread thread_;

public:
    joining_thread() noexcept = default;

    template <class Callable, class... Args>
    explicit joining_thread(Callable &&call, Args &&...args)
        : thread_(std::forward<Callable>(call), std::forward<Args>(args)...) {}

    explicit joining_thread(std::thread thread) noexcept
        : thread_(std::move(thread)) {}

    joining_thread(std::thread &&thread) noexcept : thread_(std::move(thread)) {}

    joining_thread &operator=(joining_thread &&other) noexcept
    {
        if (joinable())
        {
            join();
        }
        thread_ = std::move(other.thread_);
        return *this;
    }

    joining_thread &operator=(joining_thread other) noexcept
    {
        if (joinable())
        {
            join();
        }
        thread_ = std::move(other.thread_);
        return *this;
    }
    
    ~joining_thread() noexcept
    {
        if (joinable())
        {
            join();
        }
    }

    void swap(joining_thread &other) noexcept { thread_.swap(other.thread_); }

    std::thread::id get_id() const noexcept { return thread_.get_id(); }

    bool joinable() const noexcept { return thread_.joinable(); }

    void join() { thread_.join(); }

    void detach() { thread_.detach(); }

    std::thread &as_thread() noexcept { return thread_; }

    const std::thread &as_thread() const noexcept { return thread_; }
};

int main()
{
    std::thread task([]()
                     { std::println("threading"); }); // initial function
    background_task f;
    std::thread other_task(f);

    thread_guard guard1(task);
    thread_guard guard2(other_task);

    // passing a reference
    widget_data data;
    std::thread task3(update_data_for_widget, widget_id{}, std::ref(data));
    thread_guard guard3(task3);

    // call object method
    X x;
    joining_thread task4(&X::do_lengthy_work, &x);

    auto obj = std::make_unique<big_object>();
    scoped_thread task5(std::thread(process_big_object, std::move(obj)));
}