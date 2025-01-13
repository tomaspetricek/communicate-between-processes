#ifndef DISK_SCHEDULING_SCAN_HPP
#define DISK_SCHEDULING_SCAN_HPP

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <limits>

#include <etl/flat_set.h>

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
            constexpr request_type
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
                    if (index == requests.size())
                    {
                        curr_direction_ = head_direction::left;
                        return requests.back();
                    }
                    return requests[index];
                }
                assert(curr_direction_ == head_direction::left);

                if (index == std::size_t{0})
                {
                    curr_direction_ = head_direction::right;
                    return requests.front();
                }
                return requests[index - 1];
            }

            template <std::size_t RequestCount>
            request_type
            select_next(const disk_type &disk,
                        etl::flat_set<request_type, RequestCount> &requests) noexcept
            {
                assert(!requests.empty());

                if (curr_direction_ == head_direction::right)
                {
                    const auto first_greater = std::upper_bound(
                        requests.begin(), requests.end(), disk.head_position,
                        [](const track_number_type &target, const request_type &req)
                        {
                            return target.value < req.track_number.value;
                        });

                    if (first_greater == requests.end())
                    {
                        curr_direction_ = head_direction::left;
                        return *(--requests.end());
                    }
                    return *first_greater;
                }
                assert(curr_direction_ == head_direction::left);
                auto not_less = std::lower_bound(
                    requests.begin(), requests.end(), disk.head_position,
                    [](const request_type &req, const track_number_type &target)
                    {
                        return req.track_number.value < target.value;
                    });

                if (not_less == requests.begin())
                {
                    curr_direction_ = head_direction::right;
                    return *requests.begin();
                }
                return *(--not_less);
            }
        };
    } // namespace scan
} // namespace disk_scheduling

#endif // DISK_SCHEDULING_SCAN_HPP