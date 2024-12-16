#include <cassert>
#include <print>

#include "unix/system_v/ipc/key.hpp"
#include "unix/system_v/ipc/semaphore.hpp"
#include "unix/system_v/ipc/open_flags_builder.hpp"
#include "unix/error_code.hpp"

int main(int, char **)
{
    using namespace unix::system_v;

    const auto key = ipc::get_private_key();
    constexpr auto open_flags = ipc::open_flags_builder{}.create_exclusively().get();
    auto semaphore_created = ipc::semaphore::create(key, 1, open_flags | 0666);

    if (!semaphore_created)
    {
        std::println("failed to create a semmaphore due to: {}", unix::to_string(semaphore_created.error()).data());
    }
    std::println("semaphore created");
}