#include <print>
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

    namespace impl
    {
        static log::level curr_level = level::debug;

        template <class... Args>
        bool make_entry(const log::level &level, const std::string_view &format,
                        Args... args) noexcept
        {
            if (level < curr_level)
            {
                return true;
            }
            return true;
        }
    } // namespace impl

    void set_level(const log::level &level) noexcept { impl::curr_level = level; }

    template <class... Args>
    bool debug(const std::string_view &format, Args... args) noexcept
    {
        return impl::make_entry(level::debug, format, std::forward<Args>(args)...);
    }

    template <class... Args>
    bool info(const std::string_view &format, Args... args) noexcept
    {
        return impl::make_entry(level::info, format, std::forward<Args>(args)...);
    }

    template <class... Args>
    bool warn(const std::string_view &format, Args... args) noexcept
    {
        return impl::make_entry(level::warn, format, std::forward<Args>(args)...);
    }

    template <class... Args>
    bool error(const std::string_view &format, Args... args) noexcept
    {
        return impl::make_entry(level::error, format, std::forward<Args>(args)...);
    }

    template <class... Args>
    bool fatal(const std::string_view &format, Args... args) noexcept
    {
        return impl::make_entry(level::fatal, format, std::forward<Args>(args)...);
    }
} // namespace log

int main(int, char **)
{
    std::println("create logger");
    log::info("hello world");
}