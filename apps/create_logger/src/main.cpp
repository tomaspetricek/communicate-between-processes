#include <algorithm>
#include <iostream>
#include <print>
#include <span>
#include <string>
#include <string_view>
#include <utility>

namespace log
{
    enum class level
    {
        debug,
        info,
        warn,
        error,
        fatal,
        off
    };

    using token_t = std::span<const char>;

    class logger
    {
    public:
        bool log(const token_t &token) noexcept
        {
            std::cout.write(token.data(), token.size());
            return true;
        }

        bool end_line() noexcept
        {
            std::cout << '\n';
            return true;
        }
    };

    namespace impl
    {
        static log::level curr_level{level::debug};
        static log::logger curr_logger{};

        static constexpr char placeholder_begin{'{'};
        static constexpr char placeholder_end{'}'};

        bool find_next(std::size_t &index, const token_t &format,
                       char target) noexcept
        {
            for (; index != format.size() && format[index] != target; index++)
            {
            }
            return index != format.size();
        }

        template <class First, class... Rest>
        bool format_log(const token_t &format, const First &first,
                        Rest &&...rest) noexcept
        {
            std::size_t index{0};

            // reach next format placeholder
            if (!find_next(index, format, placeholder_begin))
            {
                return false;
            }
            // log before placeholder
            if (!curr_logger.log(token_t{format.data(), index}))
            {
                return false;
            }
            // format argument
            const auto token = std::to_string(first);

            // log argument
            if (!curr_logger.log(token_t{token.data(), token.size()}))
            {
                return false;
            }
            // pass format placeholder
            if (!find_next(++index, format, placeholder_end))
            {
                return false;
            }
            constexpr auto all_arguments_formatted = sizeof...(rest) == std::size_t{0};

            // reached end of format
            if (index == (format.size() - 1))
            {
                return all_arguments_formatted;
            }
            const auto first_index = ++index;

            // reach next format placeholder
            const auto next_found = find_next(index, format, placeholder_begin);

            // log after placeholder
            if (!curr_logger.log(format.subspan(first_index, index - first_index)))
            {
                return false;
            }
            // all arguments formatted
            if constexpr (all_arguments_formatted)
            {
                // true, an extra placeholder has been provided
                return !next_found;
            }
            else
            {
                // insufficient number of placeholders
                if (!next_found)
                {
                    return false;
                }
                // log next argument
                const auto format_written_count{index};
                return format_log(format.subspan(format_written_count,
                                                 format.size() - format_written_count),
                                  std::forward<Rest>(rest)...);
            }
        }

        template <class... Args>
        bool log(const log::level &level, const std::string_view &format,
                 Args &&...args) noexcept
        {
            // check log level sufficiency
            if (level < curr_level)
            {
                return true;
            }
            // no arguments provided
            if constexpr (sizeof...(args) == std::size_t{0})
            {
                if (!curr_logger.log(token_t{format.data(), format.size()}))
                {
                    return false;
                }
            }
            else
            {
                // format provided arguments
                if (!format_log(token_t{format.data(), format.size()},
                                std::forward<Args>(args)...))
                {
                    return false;
                }
            }
            return curr_logger.end_line();
        }
    } // namespace impl

    void set_level(const log::level &level) noexcept { impl::curr_level = level; }

    template <class... Args>
    bool debug(const std::string_view &format, Args... args) noexcept
    {
        return impl::log(level::debug, format, std::forward<Args>(args)...);
    }

    template <class... Args>
    bool info(const std::string_view &format, Args... args) noexcept
    {
        return impl::log(level::info, format, std::forward<Args>(args)...);
    }

    template <class... Args>
    bool warn(const std::string_view &format, Args... args) noexcept
    {
        return impl::log(level::warn, format, std::forward<Args>(args)...);
    }

    template <class... Args>
    bool error(const std::string_view &format, Args... args) noexcept
    {
        return impl::log(level::error, format, std::forward<Args>(args)...);
    }

    template <class... Args>
    bool fatal(const std::string_view &format, Args... args) noexcept
    {
        return impl::log(level::fatal, format, std::forward<Args>(args)...);
    }
} // namespace log

int main(int, char **)
{
    std::println("create logger");
    log::info("provide no arguments");
    log::info("provide single argument: {}", 42);
    log::info("provide multiple multiple arguents: first: {}, second: {}", 10,
              2.F);
}