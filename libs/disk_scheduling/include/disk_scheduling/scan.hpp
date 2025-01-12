#ifndef DISK_SCHEDULING_SCAN_HPP
#define DISK_SCHEDULING_SCAN_HPP

#include <algorithm>
#include <array>
#include <cassert>
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
    };

    template <class TrackNumber>
    struct request
    {
        static_assert(
            std::is_same_v<TrackNumber, track_number<TrackNumber::disk_info>>);
        TrackNumber track_number;
    };

    enum class head_direction
    {
        left,
        right
    };

    namespace scan
    {
        // time complexity:  O(N) where N is number of requests
        // space complexity: O(1)
        template <class TrackNumber, std::size_t RequestCount>
        std::size_t
        compute_seek_distance(const std::array<request<TrackNumber>, RequestCount> &requests,
                              TrackNumber init_head_position,
                              head_direction direction) noexcept
        {
            if (requests.empty())
            {
                return std::size_t{0};
            }
            const auto [min, max] = std::minmax_element(
                requests.begin(), requests.end(), [](const auto &fst, const auto &snd)
                { return fst.track_number.value < snd.track_number.value; });

            // no need to move
            if ((min->track_number.value == max->track_number.value) &&
                (min->track_number.value == init_head_position.value))
            {
                return std::size_t{0};
            }
            // need to move
            if (direction == head_direction::left)
            {
                const auto left_distance =
                    init_head_position.value - TrackNumber::min_value;
                const auto right_distance =
                    (init_head_position.value < max->track_number.value)
                        // need to go to the right
                        ? (max->track_number.value - TrackNumber::min_value)
                        : typename TrackNumber::value_type{0};
                return static_cast<std::size_t>(left_distance + right_distance);
            }
            assert(direction == head_direction::right);
            const auto right_distance = TrackNumber::max_value - init_head_position.value;
            const auto left_distance =
                (init_head_position.value > min->track_number.value)
                    // need to go to the left
                    ? (TrackNumber::max_value - min->track_number.value)
                    : typename TrackNumber::value_type{0};
            return static_cast<std::size_t>(left_distance + right_distance);
        }
    } // namespace scan
} // namespace disk_scheduling

#endif // DISK_SCHEDULING_SCAN_HPP