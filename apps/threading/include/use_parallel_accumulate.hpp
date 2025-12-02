#ifndef THREADING_USE_PARALLEL_ACCUMULATE_HPP
#define THREADING_USE_PARALLEL_ACCUMULATE_HPP

#include <print>
#include <vector>

#include "parallel_accumulate.hpp"

void use_parallel_accumulate() {
        std::vector<int> vals(1'000);

    for (std::size_t i{0}; i < vals.size(); i++)
    {
        vals[i] = static_cast<int>(i);
    }
    const auto res = parallel_accumulate(vals.cbegin(), vals.cend(), 0L);
    std::println("sum: {}", res);
}

#endif // THREADING_USE_PARALLEL_ACCUMULATE_HPP