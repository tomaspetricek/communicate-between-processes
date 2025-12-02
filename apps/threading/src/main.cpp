#include <memory>
#include <print>
#include <thread>
#include <algorithm>
#include <numeric>
#include <mutex>
#include <list>
#include <exception>
#include "use_shared_mutex.hpp"
#include "joining_thread.hpp"
#include "scoped_thread.hpp"
#include "thread_guard.hpp"
#include "parallel_accumulate.hpp"
#include "hierarchical_mutex.hpp"
#include "threadsafe_stack.hpp"
#include "use_shared_resource.hpp"
#include "lazy_initialize.hpp"
#include "use_unique_lock.hpp"
#include "use_mutex.hpp"
#include "use_threads.hpp"
#include "use_parallel_accumulate.hpp"
#include "wait_for_condition.hpp"
#include "use_threadsafe_queue.hpp"
#include "wait_for_event.hpp"


int main()
{
    use_threads();
    use_parallel_accumulate();
    wait_for_event();
}