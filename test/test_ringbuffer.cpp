#include "../source/taskete/lockfree_ringbuffer.hpp"

#include <doctest.h>

TEST_SUITE("Ringbuffer - Single Thread")
{
    using taskete::detail::LockfreeRingbuffer;

    constexpr std::uint32_t ring_size = 8;

    TEST_CASE("Just constructed buffer is empty and has correct size")
    {
        LockfreeRingbuffer<int> ring(std::pmr::get_default_resource(), ring_size);

        REQUIRE(ring.empty());
        REQUIRE(ring.size() == ring_size);
    }

    TEST_CASE("Empty buffer can be filled up completely")
    {
        LockfreeRingbuffer<int> ring(std::pmr::get_default_resource(), ring_size);

        auto sz = ring.size();
        while (sz--)
            REQUIRE(ring.try_push(sz));
    }

    TEST_CASE("A full buffer prevents additional data to be pushed")
    {
        LockfreeRingbuffer<int> ring(std::pmr::get_default_resource(), ring_size);
        auto sz = ring.size();
        while (sz--)
            REQUIRE(ring.try_push(sz));

        int test_sz = ring_size * 2 + 3;
        while (test_sz--)
            REQUIRE_FALSE(ring.try_push(test_sz));
    }

    TEST_CASE("It's not possible to pull from an empty buffer")
    {
        LockfreeRingbuffer<int> ring(std::pmr::get_default_resource(), ring_size);
        int elem = -1;
        auto sz = ring.size() * 3;
        while (sz--)
            REQUIRE_FALSE(ring.try_pull(elem));
    }

    TEST_CASE("Clearing a non-empty buffer makes it empty")
    {
        LockfreeRingbuffer<int> ring(std::pmr::get_default_resource(), ring_size);

        ring.try_push(0);
        ring.try_push(0);
        ring.try_push(0);

        REQUIRE_FALSE(ring.empty());

        ring.clear();

        REQUIRE(ring.empty());
    }
}