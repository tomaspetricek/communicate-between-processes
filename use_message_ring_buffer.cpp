#include <cassert>
#include <print>
#include <string_view>

#include "lock_free/message_ring_buffer.hpp"
#include "lock_free/ring_buffer.hpp"

int main(int, char **)
{
    lock_free::ring_buffer<std::int32_t, 2 + 1> buffer;
    assert(buffer.empty());
    assert(buffer.try_push(1));
    assert(buffer.try_push(2));
    assert(buffer.full());
    assert(!buffer.try_push(3));

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
            std::println("output message 1 ({}): {:s}", out_msg1.size(),
                         std::span<char>{out_msg1.data(), out_msg1.size()});
        }
        assert(queue.empty());
    }
}