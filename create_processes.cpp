#include <cassert>
#include <print>
#include <cstdlib>

#include "unix/posix/process.hpp"
#include "unix/error_code.hpp"


int main(int, char **)
{
    constexpr std::size_t process_to_create_count = 10;
    std::size_t child_processes_count{0};

    for (std::size_t index{0}; index < process_to_create_count; ++index)
    {
        const auto process_created = unix::posix::create_process();

        if (!process_created)
        {
            std::print("failed to create process due to: {}", unix::to_string(process_created.error()).data());
            std::abort();
        }
        const auto process_id = process_created.value();

        if (unix::posix::is_child_process(process_id))
        {
            std::println("current process id: {}", unix::posix::get_process_id());
            std::println("current process parent id: {}", unix::posix::get_parent_process_id());
            return EXIT_SUCCESS;
        }
        else
        {
            std::println("child process with id: {} created", process_id);
            std::println("index: {}", index);
            child_processes_count++;
        }
    }
    std::println("child processes count: {}", child_processes_count);
    assert(child_processes_count == process_to_create_count);
    return EXIT_SUCCESS;
}