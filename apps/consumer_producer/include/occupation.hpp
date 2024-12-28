#ifndef OCCUPATION_HPP
#define OCCUPATION_HPP

#include <variant>

#include "occupation/consumer.hpp"
#include "occupation/producer.hpp"


namespace
{
    struct no_occupation
    {
        bool run() noexcept { return true; }
    };

    using occupation_t = std::variant<no_occupation, occupation::producer, occupation::consumer>;
}

#endif // OCCUPATION_HPP