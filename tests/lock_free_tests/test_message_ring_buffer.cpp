#include <print>
#include <ranges>
#include <span>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

#include "lock_free/message_ring_buffer.hpp"

TEST(MessageRingBuffer, TestPushingPoppingMessages)
{
    constexpr std::string_view in_msg1{"pie apple pen"};
    constexpr std::size_t it_count{10}, message_count{3};
    constexpr std::size_t buffer_size{
        (message_count * (sizeof(std::size_t) + in_msg1.size())) + 1};
    lock_free::message_ring_buffer<buffer_size> queue;
    std::vector<char> out_msg1;
    out_msg1.reserve(in_msg1.size());

    for (std::size_t i{0}; i < it_count; ++i)
    {
        assert(queue.empty());

        for (std::size_t m{0}; m < message_count; ++m)
        {
            assert(!queue.full());
            assert(queue.try_push(in_msg1));
            assert(!queue.empty());
        }
        assert(queue.full());

        for (std::size_t m{0}; m < message_count; ++m)
        {
            assert(!queue.empty());
            assert(queue.try_pop(out_msg1));
            assert(!queue.full());
            const auto expect = std::span<const char>(in_msg1.data(), in_msg1.size());
            const auto actual =
                std::span<const char>(out_msg1.data(), out_msg1.size());

            ASSERT_EQ(expect.size(), actual.size());

            for (const auto &[e, a] : std::views::zip(expect, actual))
            {
                ASSERT_EQ(e, a);
            }
        }
        assert(queue.empty());
    }
}