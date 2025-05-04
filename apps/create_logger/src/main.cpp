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

        template <class First, class... Rest>
        bool create_log(const token_t &format, const First &first,
                        Rest &&...rest) noexcept
        {
            std::size_t index{0};

            for (; index != format.size() && format[index] != '{'; index++)
            {
            }
            if (index == format.size())
            {
                return false;
            }
            if (!curr_logger.log(token_t{format.data(), index}))
            {
                return false;
            }
            const auto token = std::to_string(first);

            if (!curr_logger.log(token_t{token.data(), token.size()}))
            {
                return false;
            }
            if constexpr (sizeof...(rest) == std::size_t{0})
            {
                return true;
            }
            else
            {
                index++;

                for (; index != format.size() && format[index] != '}'; index++)
                {
                }
                if (index == format.size())
                {
                    return false;
                }
                const auto format_written_count{index + 1};
                return create_log(token_t{format.data() + format_written_count,
                                          format.size() - format_written_count},
                                  std::forward<Rest>(rest)...);
            }
            return true;
        }

        template <class... Args>
        bool log(const log::level &level, const std::string_view &format,
                 Args &&...args) noexcept
        {
            if (level < curr_level)
            {
                return true;
            }
            if constexpr (sizeof...(args) == std::size_t{0})
            {
                if (!curr_logger.log(token_t{format.data(), format.size()}))
                {
                    return false;
                }
            }
            else
            {
                if (!create_log(token_t{format.data(), format.size()},
                                std::forward<Args>(args)...))
                {
                    return false;
                }
            }
            if (!curr_logger.end_line())
            {
                return false;
            }
            return true;
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