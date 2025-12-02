#ifndef THREADING_WAIT_FOR_EVENT_HPP
#define THREADING_WAIT_FOR_EVENT_HPP

#include <future>
#include <print>

int find_answer_to_everything() { return 42; }
void do_other_stuff();

void wait_for_event()
{
    auto the_answer = std::async(find_answer_to_everything);
    std::println("the answer to everything is: {}", the_answer.get());
}

#endif // THREADING_WAIT_FOR_EVENT_HPP