#ifndef BUFFERING_PROCESSOR_HPP
#define BUFFERING_PROCESSOR_HPP

#include <variant>

#include "buffering/role.hpp"
#include "buffering/occupation.hpp"

namespace buffering
{
    class processor
    {
    public:
        explicit processor(const role_t &role,
                           const occupation_t &occupation) noexcept
            : role_{role}, occupation_{occupation} {}

        bool process() noexcept
        {
            if (!std::visit([](const auto &r)
                            { return r.setup(); }, role_))
            {
                return false;
            }
            if (!std::visit([](auto &o)
                            { return o.run(); }, occupation_))
            {
                return false;
            }
            if (!std::visit([](auto &r)
                            { return r.finalize(); }, role_))
            {
                return false;
            }
            return true;
        }

    private:
        role_t role_;
        occupation_t occupation_;
    };
}

#endif // BUFFERING_PROCESSOR_HPP