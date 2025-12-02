#include <memory>
#include <print>
#include <thread>
#include <algorithm>
#include <numeric>
#include <mutex>
#include <list>
#include <exception>
#include "use_shared_mutex.hpp"

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

template <class Iterator, class T>
struct accumulate_block
{
    void operator()(Iterator first, Iterator last, T &result)
    {
        result = std::accumulate(first, last, result);
    }
};

template <class Iterator, class T>
T parallel_accumulate(Iterator first, Iterator last, T init)
{
    const auto length = std::distance(first, last);

    if (!length)
    {
        return init;
    }
    const unsigned long min_per_thread = 25;
    const unsigned long max_threads = (length + min_per_thread - 1) / min_per_thread;
    const unsigned long hardware_threads = std::thread::hardware_concurrency();

    const unsigned long num_threads = std::min(hardware_threads != 0 ? hardware_threads : 2, max_threads);
    const unsigned long block_size = length / num_threads;
    std::vector<T> results(num_threads);
    std::vector<std::thread> threads(num_threads - 1);

    Iterator block_start = first;

    for (unsigned long i = 0; i < threads.size(); i++)
    {
        Iterator block_end = block_start;
        std::advance(block_end, block_size);
        threads[i] = std::thread(accumulate_block<Iterator, T>{}, block_start, block_end, std::ref(results[i]));
        block_start = block_end;
    }
    accumulate_block<Iterator, T>{}(block_start, last, results.back());

    for (auto &thread : threads)
    {
        thread.join();
    }
    return std::accumulate(results.cbegin(), results.cend(), init);
}

std::list<int> some_list;
std::mutex some_mutex;

void add_to_list(int value)
{
    std::lock_guard guard(some_mutex);
    some_list.push_back(value);
}

bool list_contains(int value)
{
    std::lock_guard guard(some_mutex);
    return std::find(some_list.cbegin(), some_list.cend(), value) != some_list.cend();
}

struct empty_stack : std::exception
{
    const char *what() const throw()
    {
        return "empty stack";
    }
};

template <class T>
class threadsafe_stack
{
    std::stack<T> data;
    mutable std::mutex mutex;

public:
    threadsafe_stack() = default;

    threadsafe_stack(const threadsafe_stack &other)
    {
        std::lock_guard lock(other.mutex);
        data = other.data;
    }

    threadsafe_stack &operator=(const threadsafe_stack &) = delete;

    void push(T value)
    {
        std::lock_guard lock(mutex);
        data.push(std::move(value));
    }

    std::shared_ptr<T> pop()
    {
        std::lock_guard lock(mutex);

        if (data.empty())
        {
            throw empty_stack();
        }
        const auto res = std::make_shared(data.top());
        data.pop();
        return res;
    }

    void pop(T &value)
    {
        std::lock_guard lock(mutex);

        if (data.empty())
        {
            throw empty_stack();
        }
        value = data.top();
        data.pop();
    }

    bool empty() const
    {
        std::lock_guard lock(mutex);
        return data.empty();
    }
};

class hierarchical_mutex
{
    std::mutex internal_mutex;
    const unsigned long hierarchy_value;
    unsigned long previous_hierarchy_value;
    static thread_local unsigned long this_thread_hierarchy_value;

    void check_for_hierarchy_violation()
    {
        if (this_thread_hierarchy_value <= hierarchy_value)
        {
            throw std::logic_error("mutex hierarchy violated");
        }
    }

    void update_hierarchy_value()
    {
        previous_hierarchy_value = this_thread_hierarchy_value;
        this_thread_hierarchy_value = hierarchy_value;
    }

public:
    explicit hierarchical_mutex(unsigned long value) : hierarchy_value(value), previous_hierarchy_value(0) {}

    void lock()
    {
        check_for_hierarchy_violation();
        internal_mutex.lock();
        update_hierarchy_value();
    }

    void unlock()
    {
        if (this_thread_hierarchy_value != hierarchy_value)
        {
            throw std::logic_error("mutex hierarchy violated");
        }
        this_thread_hierarchy_value = previous_hierarchy_value;
        internal_mutex.unlock();
    }

    bool try_lock()
    {
        check_for_hierarchy_violation();

        if (!internal_mutex.try_lock())
        {
            return false;
        }
        update_hierarchy_value();
        return true;
    }
};
thread_local unsigned long hierarchical_mutex::this_thread_hierarchy_value(ULONG_MAX);

class some_big_object
{
};
void swap(some_big_object &lhs, some_big_object &rhs) {}

class Y
{
    some_big_object some_detail;
    std::mutex m;

public:
    Y(const some_big_object &some_detail) : some_detail(some_detail) {}

    friend void swap(Y &rhs, Y &lhs)
    {
        if (&rhs == &lhs)
        {
            return;
        }
        std::unique_lock lock_rhs(rhs.m, std::defer_lock);
        std::unique_lock lock_lhs(lhs.m, std::defer_lock);
        std::lock(lock_rhs, lock_lhs);
        swap(rhs.some_detail, lhs.some_detail);
    }
};

struct some_resource
{
    void do_something() {}
};

std::shared_ptr<some_resource> resource_ptr;
std::once_flag resource_flag;

void init_resource()
{
    resource_ptr.reset(new some_resource);
}

void use_shared_resource()
{
    // lazy initialization
    std::call_once(resource_flag, init_resource);
    resource_ptr->do_something();
}

struct data_packet
{
};
struct connection_info
{
};
struct connection_handle
{
    void send_data(const data_packet &packet) {}
    data_packet receive_data() {}
};
struct connection_manager
{
    static connection_handle open(const connection_info &info)
    {
        return connection_handle();
    }
};

class Z
{
    connection_info connection_details;
    connection_handle connection;
    std::once_flag connection_init_flag;

    void open_connection()
    {
        connection = connection_manager::open(connection_details);
    }

public:
    Z(const connection_info &info) : connection_details(info) {}

    void send_data(const data_packet &packet)
    {
        std::call_once(connection_init_flag, &Z::open_connection, this); // lazy initialization
        connection.send_data(packet);
    }

    data_packet receive_data()
    {
        std::call_once(connection_init_flag, &Z::open_connection, this); // lazy initialization
        return connection.receive_data();
    }
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

    std::vector<int> vals(1'000);

    for (std::size_t i{0}; i < vals.size(); i++)
    {
        vals[i] = static_cast<int>(i);
    }
    const auto res = parallel_accumulate(vals.cbegin(), vals.cend(), 0L);
    std::println("sum: {}", res);

    // use shared mutex
    dns_chache chache;
}