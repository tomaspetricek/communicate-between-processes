#ifndef BUFFERING_OCCUPATION_HPP
#define BUFFERING_OCCUPATION_HPP

#include <variant>

#include "buffering/occupation/consumer.hpp"
#include "buffering/occupation/producer.hpp"

namespace buffering
{
    struct no_occupation
    {
        bool run() noexcept { return true; }
    };

    using occupation_t = std::variant<no_occupation, occupation::producer, occupation::consumer>;
}

#endif // BUFFERING_OCCUPATION_HPP