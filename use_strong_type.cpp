#include <cstdint>
#include <print>

#include <strong_type/strong_type.hpp>

using meters_t = strong::type<std::int32_t, struct meter_, strong::arithmetic, strong::formattable>;

int main(int, char **)
{
    meters_t in_kilometer{1'000};
    std::println("number of meter in kilometer: {}", in_kilometer);
}