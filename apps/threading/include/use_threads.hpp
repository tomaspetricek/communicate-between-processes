#ifndef THREADING_USE_THREADS_HPP
#define THREADING_USE_THREADS_HPP

#include <thread>
#include <print>

#include "thread_guard.hpp"
#include "joining_thread.hpp"
#include "thread_guard.hpp"
#include "scoped_thread.hpp"

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

void use_threads()
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

#endif // THREADING_USE_THREADS_HPP