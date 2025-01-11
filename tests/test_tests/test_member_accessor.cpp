#include <cstdint>

#include <gtest/gtest.h>

#include "test/member_accessor.hpp"

class treasure_keeper
{
public:
    using treasure_type = std::int32_t;

    explicit treasure_keeper(treasure_type treasure) noexcept
        : treasure_{treasure} {}

private:
    treasure_type treasure_{42};
};

using treasure_keepers_treasure =
    test::member_accessor_tag<treasure_keeper, treasure_keeper::treasure_type>;
template struct test::member_accessor<treasure_keepers_treasure,
                                      &treasure_keeper::treasure_>;

TEST(MemberAccessor, AccessTreasure)
{
    constexpr treasure_keeper::treasure_type treasure{42};
    const auto keeper = treasure_keeper{treasure};
    ASSERT_EQ(treasure, test::access_member<treasure_keepers_treasure>(keeper));
}