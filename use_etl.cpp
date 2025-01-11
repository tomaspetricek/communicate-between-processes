#include <print>

#include <etl/array.h>

int main(int, char **)
{
    constexpr std::size_t num_count{3};
    constexpr etl::array<std::int32_t, num_count> nums{1, 2, 3};

    for (const auto &num : nums)
    {
        std::print("{},", num);
    }
    std::println("");
}