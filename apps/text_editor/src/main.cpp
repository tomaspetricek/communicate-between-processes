#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <optional>
#include <print>
#include <termios.h>
#include <thread>
#include <unistd.h>


    struct position
    {
        int row{0}, col{0};
    };


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

    // ESC [ <row> ; <col> R
    std::optional<position> get_cursor_position()
    {
        const auto position_requested = write_text("\x1b[6n");

        if (!position_requested)
        {
            return std::nullopt;
        }
        char buf[32];
        unsigned int i{0};

        bool read_all{false};

        for (; i < sizeof(buf) - 1; i++)
        {
            const auto key = read_key();

            if (!key.has_value())
            {
                return std::nullopt;
            }
            buf[i] = key.value();

            if (buf[i] == 'R')
            {
                read_all = true;
                break;
            }
        }
        if (!read_all)
        {
            return std::nullopt;
        }
        if (i < 2)
        {
            return std::nullopt;
        }
        if (buf[0] != '\x1b' || buf[1] != '[')
        {
            return std::nullopt;
        }
        position pos;

        if (sscanf(&buf[2], "%d;%d", &pos.row, &pos.col) != 2)
        {
            return std::nullopt;
        }
        return pos;
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
    int32_t count_{0};

    void process_key_in_edit_mode(char key)
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

    void process_key_in_view_mode(char key)
    {
        if (key == 'q')
        {
            keep_running_ = false;
        }
        if (key == 'e')
        {
            edit_mode_ = true;
        }
    }

    bool display_menu_in_view_mode()
    {
        if (!console::move_cursor_home())
        {
            return false;
        }
        return console::write_text(
            "\n\npress: q - to quit, e - to continue editing");
    }

    bool display_document()
    {
        if (count_ % 2 == 0)
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
        return true;
    }

    void process_key(char key)
    {
        if (edit_mode_)
        {
            process_key_in_edit_mode(key);
        }
        else
        {
            process_key_in_view_mode(key);
        }
    }

    bool display_cursor_position()
    {
        const auto position_retrived = console::get_cursor_position();

        if (!position_retrived.has_value())
        {
            return false;
        }
        const auto &pos = position_retrived.value();

        char buf[100];
        const int needed = snprintf(buf, sizeof(buf),
                                    "\ncurrent cursor position: row: %d, col: %d",
                                    pos.row, pos.col);

        // error occured
        if (needed < 0)
        {
            return false;
        }
        // truncated
        if (needed >= sizeof(buf))
        {
            return false;
        }
        if (!console::write_text(buf))
        {
            return false;
        }
        return true;
    }

public:
    bool run()
    {
        struct termios orig_termios;

        if (!console::enable_raw_mode(orig_termios))
        {
            return false;
        }
        while (keep_running_)
        {
            const auto key_read = console::read_key();

            if (!key_read.has_value())
            {
                return EXIT_FAILURE;
            }
            auto &key = key_read.value();
            process_key(key);

            if (!console::refresh_screen())
            {
                return false;
            }
            if (!keep_running_)
            {
                continue;
            }
            if (!display_document())
            {
                return false;
            }
            if (!display_cursor_position())
            {
                return false;
            }
            if (!edit_mode_)
            {
                if (!display_menu_in_view_mode())
                {
                    return false;
                }
            }
            count_++;
        }
        if (!console::disable_raw_mode(orig_termios))
        {
            return false;
        }
        return true;
    }
};

int main() { return text_editor{}.run() ? EXIT_SUCCESS : EXIT_FAILURE; }