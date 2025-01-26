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

        constexpr bool read(char &value) noexcept
        {
            if (index_ == buffer_.size())
            {
                return false;
            }
            value = buffer_[index_++];
            return true;
        }

        constexpr bool empty() const noexcept
        {
            return index_ == buffer_.size();
        }

        void reset() noexcept
        {
            index_ = 0;
        }
    };
} // namespace kaleidoscope

#endif // KALEIDOSCOPE_BUFFER_READER_HPP