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

    std::string make_bold(const std::string_view &text)
    {
        std::string result;
        const std::string_view start{"\x1b[1m"};
        const std::string_view end{"\x1b[0m"};
        result.reserve(start.size() + text.size() + end.size());
        result.append(start);
        result.append(text);
        result.append(end);
        return result;
    }
} // namespace console

namespace keys
{
    static constexpr int DELETE = 127;
    static constexpr int BACKSPACE = 8;
    static constexpr int ESCAPE = 27;
} // namespace keys

class text_editor
{
    std::string document_;
    bool keep_running_{true};
    bool edit_mode_{true};

public:
    bool run()
    {
        struct termios orig_termios;

        if (!console::enable_raw_mode(orig_termios))
        {
            return false;
        }
        int32_t count{0};

        while (keep_running_)
        {
            const auto key_read = console::read_key();

            if (!key_read.has_value())
            {
                return EXIT_FAILURE;
            }
            auto &key = key_read.value();

            if (edit_mode_)
            {
                if (key == keys::DELETE || key == keys::BACKSPACE)
                {
                    if (!document_.empty())
                    {
                        document_.pop_back();
                    }
                }
                else if (key == keys::ESCAPE)
                {
                    edit_mode_ = false;
                }
                else
                {
                    document_.push_back(key);
                }
            }
            else
            {
                if (key == 'q')
                {
                    keep_running_ = false;
                    continue;
                }
            }
            if (!console::refresh_screen())
            {
                return false;
            }

            if (count % 2 == 0)
            {
                if (!console::write_text(document_))
                {
                    return false;
                }
            }
            else
            {
                if (!console::write_text(console::make_bold(document_)))
                {
                    return false;
                }
            }
            if (!edit_mode_)
            {
                if (!console::move_cursor_home())
                {
                    return false;
                }
                if (!console::write_text("\npress q to quit"))
                {
                    return false;
                }
            }
            count++;
        }
        if (!console::disable_raw_mode(orig_termios))
        {
            return false;
        }
        return true;
    }
};

int main() { return text_editor{}.run() ? EXIT_SUCCESS : EXIT_FAILURE; }