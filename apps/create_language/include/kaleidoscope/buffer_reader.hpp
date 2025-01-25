#ifndef KALEIDOSCOPE_BUFFER_READER_HPP
#define KALEIDOSCOPE_BUFFER_READER_HPP

#include <string_view>

namespace kaleidoscope
{
    class buffer_reader
    {
        std::string_view buffer_;
        std::size_t index_{0};

    public:
        constexpr explicit buffer_reader(const std::string_view &buffer) noexcept
            : buffer_{buffer} {}

        constexpr char read() noexcept
        {
            if (index_ == buffer_.size())
            {
                return EOF;
            }
            return buffer_[index_++];
        }

        void reset() noexcept {
            index_ = 0;
        }
    };
} // namespace kaleidoscope

#endif // KALEIDOSCOPE_BUFFER_READER_HPP