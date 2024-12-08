#include <print>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

namespace posix
{
    using pipe_file_descriptor_t = int;
    constexpr std::size_t pipe_count = 2;
    constexpr std::size_t pipe_read_index = 0;
    constexpr std::size_t pipe_write_index = 1;

    constexpr bool is_child_process(pid_t pid) noexcept
    {
        return pid == 0;
    }
} // namespace posix

namespace
{
    int send_message_between_processes() noexcept
    {
        posix::pipe_file_descriptor_t pipefd[posix::pipe_count];

        // Create a pipe
        if (pipe(pipefd) == -1)
        {
            perror("pipe");
            return 1;
        }

        // Fork the process
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
            close(pipefd[posix::pipe_write_index]);
            char buffer[128];
            read(pipefd[posix::pipe_read_index], buffer, sizeof(buffer));
            printf("child received: %s\n", buffer);
            close(pipefd[0]);
        }
        // parent process
        else
        {
            // close unused read end
            close(pipefd[posix::pipe_read_index]);
            char message[] = "Hello, from parent";
            write(pipefd[posix::pipe_write_index], message, sizeof(message));
            close(pipefd[posix::pipe_write_index]);
        }
        return EXIT_SUCCESS;
    }
}

int main(int, char **)
{
    return send_message_between_processes();
}
