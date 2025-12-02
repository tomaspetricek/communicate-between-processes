#ifndef THREADING_USE_MUTEX_HPP
#define THREADING_USE_MUTEX_HPP

#include <list>
#include <mutex>

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

#endif // THREADING_USE_MUTEX_HPP