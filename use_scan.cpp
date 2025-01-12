#include <print>

#include "disk_scheduling/scan.hpp"

int main(int, char **)
{
    constexpr auto disk_info = disk_scheduling::type_info<std::int32_t>{0, 199};
    using track_number_t = disk_scheduling::track_number<disk_info>;
    using request_t = disk_scheduling::request<track_number_t>;

    auto requests = std::array{
        request_t{track_number_t{176}}, request_t{track_number_t{79}},
        request_t{track_number_t{34}}, request_t{track_number_t{60}},
        request_t{track_number_t{92}}, request_t{track_number_t{11}},
        request_t{track_number_t{41}}, request_t{track_number_t{114}}};
    const track_number_t init_head_position{50};
    const auto seek_distance = disk_scheduling::scan::compute_seek_distance(
        requests, init_head_position, disk_scheduling::head_direction::left);
    std::println("seek distance: {}", seek_distance);

    disk_scheduling::scan::scheduler<disk_info> scheduler{disk_scheduling::head_direction::left};
    disk_scheduling::disk<disk_info> disk{track_number_t{8}};
    const auto selected = scheduler.select_next(disk, requests);
    std::println("selected: {}", selected.track_number.value);
}