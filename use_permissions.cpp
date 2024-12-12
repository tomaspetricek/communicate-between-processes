#include <cassert>

#include "posix/permissions_builder.hpp"


int main(int, char **)
{
    const auto perms = posix::permissions_builder{}
                           .owner_can_read()
                           .owner_can_write()
                           .owner_can_execute()
                           .group_can_read()
                           .group_can_write()
                           .group_can_execute()
                           .others_can_read()
                           .others_can_write()
                           .others_can_execute()
                           .get();
    assert(0777 == perms);
}