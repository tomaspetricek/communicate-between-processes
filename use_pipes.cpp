#include <cassert>
#include <print>
#include <stdio.h>
#include <string.h>

#include "unix/posix/ipc/pipe.hpp"
#include "unix/posix/process.hpp"

namespace
{
    int send_message_between_processes() noexcept
    {
        auto pipe_created = unix::posix::ipc::pipe::create();

        if (!pipe_created)
        {
            perror("pipe");
            return EXIT_FAILURE;
        }
        auto& pipe = pipe_created.value();

        // fork the process
        const auto process_created = unix::posix::create_process();

        if (!process_created)
        {
            std::print("failed to create process due to: {}", unix::to_string(process_created.error()).data());
            return EXIT_FAILURE;
        }
        // child process
        else if (unix::posix::is_child_process(process_created.value()))
        {
            // close unused write end
            assert(pipe.close_write_end());
            char buffer[128];
            assert(pipe.read(buffer, sizeof(buffer)));
            printf("child received: %s\n", buffer);
            assert(pipe.close_read_end());
        }
        // parent process
        else
        {
            // close unused read end
            assert(pipe.close_read_end());
            char message[] = "Hello, from parent";
            assert(pipe.write(message, sizeof(message)));
            assert(pipe.close_write_end());
        }
        return EXIT_SUCCESS;
    }
}

int main(int, char **)
{
    return send_message_between_processes();
}
