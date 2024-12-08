#include <print>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <expected>
#include <array>
#include <cassert>
#include <bitset>

namespace posix
{

    constexpr bool is_child_process(pid_t pid) noexcept
    {
        return pid == 0;
    }

    enum class error_code
    {
        creation,
    };

    class pipe
    {
        using file_descriptor_t = int;
        static constexpr std::size_t end_count = 2;
        static constexpr std::size_t read_end_index = 0;
        static constexpr std::size_t write_end_index = 1;
        using file_descriptors_t = std::array<file_descriptor_t, end_count>;

    public:
        static std::expected<posix::pipe, error_code> create() noexcept
        {
            file_descriptor_t fds[end_count];

            if (::pipe(fds) == -1)
            {
                return std::unexpected{error_code::creation};
            }
            return posix::pipe{std::to_array(fds)};
        }

        pipe(const pipe &other) = delete;
        pipe &operator=(const pipe &other) = delete;

        bool is_end_open(std::size_t index) const
        {
            assert(index < end_count);
            return end_open_ & (1 << index);
        }

        bool is_read_end_open() const
        {
            return is_end_open(read_end_index);
        }

        bool is_write_end_open() const
        {
            return is_end_open(write_end_index);
        }

        void close_end(std::size_t index)
        {
            assert(index < end_count);
            assert(is_end_open(index));
            close(fds_[index]);
            end_open_ &= ~(1 << index); // set bit to 0
        }

        pipe(pipe &&other) noexcept
            : fds_{other.fds_}, end_open_{other.end_open_}
        {
            other.end_open_ = 0;
        }

        pipe &operator=(pipe &&other) noexcept
        {
            pipe temp(std::move(other));
            other.end_open_ = 0;
            return *this;
        }

        void write(void *data, size_t count) const noexcept
        {
            assert(is_write_end_open());
            ::write(fds_[write_end_index], data, count);
        }

        void read(void *data, size_t count) const noexcept
        {
            assert(is_read_end_open());
            ::read(fds_[read_end_index], data, count);
        }

        void close_read_end() noexcept
        {
            close_end(read_end_index);
        }

        void close_write_end() noexcept
        {
            close_end(write_end_index);
        }

        ~pipe() noexcept
        {
            if (is_read_end_open())
            {
                close_read_end();
            }
            if (is_write_end_open())
            {
                close_write_end();
            }
        }

    private:
        explicit pipe(const file_descriptors_t &fds) noexcept : fds_{fds} {}

        file_descriptors_t fds_;
        uint8_t end_open_{0b11111111};
    };
} // namespace posix

namespace
{
    int send_message_between_processes() noexcept
    {
        auto pipe_created = posix::pipe::create();

        if (!pipe_created)
        {
            perror("pipe");
            return 1;
        }
        auto pipe = std::move(pipe_created).value();

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
