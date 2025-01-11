#ifndef TEST_MEMBER_ACCESSOR_HPP
#define TEST_MEMBER_ACCESSOR_HPP

#include <cassert>

#include <functional>


// src: https://youtu.be/kgE8v5M1Eoo
namespace test
{
    template <class Tag, auto...>
    struct member_accessor;

    template <class Tag>
    struct member_accessor<Tag>
    {
        inline static typename Tag::type value{};
    };

    template <class Tag, typename Tag::type Member>
    struct member_accessor<Tag, Member>
    {
        inline static const decltype(Member) value = member_accessor<Tag>::value =
            Member;
    };

    template<class Tag, class Object>
    decltype(auto) access_member(Object&& object) {
        assert(member_accessor<Tag>::value);
        return std::invoke(member_accessor<Tag>::value, std::forward<Object>(object));
    }

    template<class Class, class Member, typename = void>
    struct member_accessor_tag {
        using type = Member Class::*;
    };
} // namespace test

#endif // TEST_MEMBER_ACCESSOR_HPP