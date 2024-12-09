#include <print>
#include <stdio.h>
#include <string.h>

#include "posix/pipe.hpp"
#include "posix/process.hpp"

namespace
{
    int send_message_between_processes() noexcept
    {
        auto pipe_created = posix::pipe::create();

        if (!pipe_created)
        {
            perror("pipe");
            return EXIT_FAILURE;
        }
        auto& pipe = pipe_created.value();

        // fork the process
        const pid_t pid = fork();

        if (pid == -1)
        {
            perror("fork");
            return EXIT_FAILURE;
        }
        // child process
        else if (posix::is_child_process(pid))
        {
            // close unused write end
            pipe.close_write_end();
            char buffer[128];
            pipe.read(buffer, sizeof(buffer));
            printf("child received: %s\n", buffer);
            pipe.close_read_end();
        }
        // parent process
        else
        {
            // close unused read end
            pipe.close_read_end();
            char message[] = "Hello, from parent";
            pipe.write(message, sizeof(message));
            pipe.close_write_end();
        }
        return EXIT_SUCCESS;
    }
}

int main(int, char **)
{
    return send_message_between_processes();
}
