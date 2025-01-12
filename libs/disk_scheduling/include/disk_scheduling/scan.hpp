#ifndef DISK_SCHEDULING_SCAN_HPP
#define DISK_SCHEDULING_SCAN_HPP

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <limits>

#include "disk_scheduling/disk.hpp"

namespace disk_scheduling
{
    template <auto DiskInfo>
    struct disk
    {
        using track_number_type = track_number<DiskInfo>;
        track_number_type head_position;
    };

    namespace scan
    {
        // time complexity:  O(N) where N is number of requests
        // space complexity: O(1)
        template <class TrackNumber, std::size_t RequestCount>
        std::size_t compute_seek_distance(
            const std::array<request<TrackNumber>, RequestCount> &requests,
            TrackNumber init_head_position, head_direction direction) noexcept
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

        template <auto DiskInfo>
        class scheduler
        {
            head_direction curr_direction_{head_direction::left};

        public:
            using disk_type = disk<DiskInfo>;
            using track_number_type = track_number<DiskInfo>;
            using request_type = request<track_number_type>;

            explicit scheduler(const head_direction &direction) noexcept
                : curr_direction_{direction} {}

            template <std::size_t RequestCount>
            request_type
            select_next(const disk_type &disk,
                        std::array<request_type, RequestCount> &requests) noexcept
            {
                std::sort(requests.begin(), requests.end(),
                          [](const auto &fst, const auto &snd)
                          {
                              return fst.track_number.value < snd.track_number.value;
                          });
                std::size_t index{0};

                for (; index < requests.size(); ++index)
                {
                    if (requests[index].track_number.value > disk.head_position.value)
                    {
                        break;
                    }
                }
                if (curr_direction_ == head_direction::right)
                {
                    // greater than the last
                    if (index == requests.size())
                    {
                        // flip direction
                        curr_direction_ = head_direction::left;

                        // next in the opposite direction
                        return requests.back();
                    }
                    // next in the current direction
                    return requests[index];
                }
                assert(curr_direction_ == head_direction::left);

                // less than the first
                if (index == std::size_t{0})
                {
                    // flip direction
                    curr_direction_ = head_direction::right;

                    // next in the opposite direction
                    return requests.front();
                }
                // next in the current direction
                return requests[index - 1];
            }
        };
    } // namespace scan
} // namespace disk_scheduling

#endif // DISK_SCHEDULING_SCAN_HPP