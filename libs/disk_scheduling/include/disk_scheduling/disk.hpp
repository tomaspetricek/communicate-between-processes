#ifndef DISK_SCHEDULING_DISK_HPP
#define DISK_SCHEDULING_DISK_HPP

#include <cstdint>
#include <type_traits>

namespace disk_scheduling
{
    template <class Value>
    struct type_info
    {
        using value_type = Value;
        Value min;
        Value max;
    };

    template <auto DiskInfo>
    struct track_number
    {
        using value_type = typename decltype(DiskInfo)::value_type;
        static constexpr auto disk_info = DiskInfo;
        static constexpr auto min_value = disk_info.min;
        static constexpr auto max_value = disk_info.max;

        explicit track_number(value_type value_) noexcept : value{value_}
        {
            static_assert(std::is_same_v<decltype(DiskInfo), type_info<value_type>>);
            static_assert(DiskInfo.max > DiskInfo.min);
            static_assert(DiskInfo.min >= value_type{0});
            assert(value >= DiskInfo.min);
            assert(value <= DiskInfo.max);
        }

        value_type value;

        friend bool operator<(const track_number<DiskInfo> &lhs,
                              const track_number<DiskInfo> &rhs) noexcept
        {
            return lhs.value < rhs.value;
        }
    };

    template <class TrackNumber>
    struct request
    {
        static_assert(
            std::is_same_v<TrackNumber, track_number<TrackNumber::disk_info>>);
        TrackNumber track_number;

        friend bool operator<(const request<TrackNumber> &lhs, const request<TrackNumber> &rhs) noexcept
        {
            return lhs.track_number < rhs.track_number;
        }
    };

    enum class head_direction
    {
        left,
        right
    };
} // namespace disk_scheduling

#endif // DISK_SCHEDULING_DISK_HPP