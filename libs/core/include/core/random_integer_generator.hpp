#ifndef CORE_RANDOM_INTEGER_GENERATOR_HPP
#define CORE_RANDOM_INTEGER_GENERATOR_HPP

#include <cassert>
#include <random>

namespace core
{
    template <class Integer>
    class random_integer_generator
    {
    public:
        explicit random_integer_generator(const Integer &min,
                                          const Integer &max) noexcept
            : gen_{std::random_device{}()}, distrib_{min, max}
        {
            assert(min <= max);
        }

        Integer operator()() noexcept { return distrib_(gen_); }

    private:
        std::mt19937 gen_;
        std::uniform_int_distribution<Integer> distrib_;
    };
} // namespace core

#endif // CORE_RANDOM_INTEGER_GENERATOR_HPP