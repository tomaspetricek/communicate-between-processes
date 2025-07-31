#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <optional>
#include <print>
#include <termios.h>
#include <thread>
#include <unistd.h>

namespace console
{
    bool write_text(const std::string_view &text)
    {
        return write(STDOUT_FILENO, text.data(), text.size()) == text.size();
    }

    bool write_key(char key) { return write(STDOUT_FILENO, &key, 1) == 1; }

    bool clear_screen() { return write_text("\x1b[2J"); }

    bool move_cursor_home() { return write_text("\x1b[H"); }

    bool refresh_screen() { return clear_screen() && move_cursor_home(); }

    std::optional<char> read_key()
    {
        char key{};
        const auto count = read(STDIN_FILENO, &key, 1);

        if (count != 1)
        {
            return std::nullopt;
        }
        return key;
    }

    bool disable_raw_mode(termios &orig_termios)
    {
        return tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) != -1;
    }

    bool enable_raw_mode(termios &orig_termios)
    {
        if (tcgetattr(STDIN_FILENO, &orig_termios) == -1)
        {
            std::println("failed to get terminal attributes");
            return false;
        }
        struct termios raw{
            orig_termios};
        raw.c_lflag &= ~(ECHO | ICANON | ISIG);
        raw.c_iflag &= ~(IXON | ICRNL);
        raw.c_oflag &= ~(OPOST);

        if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
        {
            std::println("failed to set terminal attributes");
            return false;
        }
        return true;
    }
} // namespace console

class text_editor
{
    std::vector<char> document;
};

int main()
{
    std::println("text editor");
    struct termios orig_termios;

    if (!console::enable_raw_mode(orig_termios))
    {
        std::println("failed to enable raw mode");
        return EXIT_FAILURE;
    }
    std::println("raw mode enabled");
    bool running{true};
    std::string text;

    while (running)
    {
        const auto key_read = console::read_key();

        if (!key_read.has_value())
        {
            return EXIT_FAILURE;
        }
        auto &key = key_read.value();

        if (key == 'q')
        {
            running = false;
            continue;
        }
        if (!console::refresh_screen())
        {
            return EXIT_FAILURE;
        }
        text.push_back(key);

        if (!console::write_text(text))
        {
            return EXIT_FAILURE;
        }
    }
    if (!console::disable_raw_mode(orig_termios))
    {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}