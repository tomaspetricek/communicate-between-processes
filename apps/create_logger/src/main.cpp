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
        bool create_log(const std::string_view &format, const First &first,
                        Rest &&...rest) noexcept
        {
            const auto token = std::to_string(first);

            if (!curr_logger.log(token_t{token.data(), token.size()}))
            {
                return false;
            }
            // ToDo: remove
            if (!curr_logger.log(token_t{" "}))
            {
                return false;
            }
            if constexpr (sizeof...(rest) == std::size_t{0})
            {
                return true;
            }
            else
            {
                return create_log(format, std::forward<Rest>(rest)...);
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
            // ToDo: use format together with args
            if (!curr_logger.log(token_t{format.data(), format.size()}))
            {
                return false;
            }
            if constexpr (sizeof...(args) != std::size_t{0})
            {
                if (!create_log(format, std::forward<Args>(args)...))
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
    log::info("hello world");
    log::info("provide multiple multiple arguents: first: {}, second: {}", 10,
              2.F);
}