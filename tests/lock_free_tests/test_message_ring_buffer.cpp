#include <print>
#include <ranges>
#include <span>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

#include "lock_free/message_ring_buffer.hpp"

template <std::size_t MessageCount>
constexpr std::size_t compute_buffer_min_capacity(
    const std::array<std::string_view, MessageCount> &messages) noexcept
{
    std::size_t res{MessageCount * sizeof(std::size_t)};

    for (const auto &msg : messages)
    {
        res += msg.size();
    }
    return res + 1;
}

using message_view = std::span<const char>;

void are_same(const message_view &actual, const message_view &expect) noexcept
{
    ASSERT_EQ(expect.size(), actual.size());

    for (const auto &[e, a] : std::views::zip(expect, actual))
    {
        ASSERT_EQ(e, a);
    }
}

TEST(MessageRingBuffer, TestPushingPoppingMessages)
{
    constexpr std::size_t message_count = 3;
    constexpr auto input_messages =
        std::array<std::string_view, message_count>{"pie", "apple", "pie"};
    constexpr auto buffer_capacity = compute_buffer_min_capacity(input_messages);
    constexpr std::size_t it_count{10};
    lock_free::message_ring_buffer<buffer_capacity> queue;
    std::vector<char> out_msg;
    out_msg.reserve(10);

    for (std::size_t i{0}; i < it_count; ++i)
    {
        assert(queue.empty());

        for (const auto &msg : input_messages)
        {
            assert(!queue.full());
            assert(queue.try_push(msg));
            assert(!queue.empty());
        }
        assert(queue.full());

        for (const auto &in_msg : input_messages)
        {
            assert(!queue.empty());
            assert(queue.try_pop(out_msg));
            assert(!queue.full());
            are_same(message_view(in_msg.data(), in_msg.size()),
                     message_view(out_msg.data(), out_msg.size()));
        }
        assert(queue.empty());
    }
}