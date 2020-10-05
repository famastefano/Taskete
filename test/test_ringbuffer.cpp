#include "../source/taskete/lockfree_ringbuffer.hpp"

#include <doctest.h>

#include <atomic>
#include <thread>

TEST_SUITE("Ringbuffer - General - Single Thread")
{
    using taskete::detail::lockfree_ringbuffer;

    constexpr std::uint32_t ring_size = 8;

    TEST_CASE("Just constructed buffer is empty and has correct size")
    {
        lockfree_ringbuffer<int> ring(std::pmr::get_default_resource(), ring_size);

        REQUIRE(ring.empty());
        REQUIRE(ring.size() == ring_size);
    }

    TEST_CASE("Empty buffer can be filled up completely")
    {
        lockfree_ringbuffer<int> ring(std::pmr::get_default_resource(), ring_size);

        auto sz = ring.size();
        while (sz--)
            REQUIRE(ring.try_push(sz));
    }

    TEST_CASE("A full buffer prevents additional data to be pushed")
    {
        lockfree_ringbuffer<int> ring(std::pmr::get_default_resource(), ring_size);
        auto sz = ring.size();
        while (sz--)
            REQUIRE(ring.try_push(sz));

        int test_sz = ring_size * 2 + 3;
        while (test_sz--)
            REQUIRE_FALSE(ring.try_push(test_sz));
    }

    TEST_CASE("It's not possible to pull from an empty buffer")
    {
        lockfree_ringbuffer<int> ring(std::pmr::get_default_resource(), ring_size);
        int elem = -1;
        auto sz = ring.size() * 3;
        while (sz--)
            REQUIRE_FALSE(ring.try_pull(elem));
    }

    TEST_CASE("Clearing a non-empty buffer makes it empty")
    {
        lockfree_ringbuffer<int> ring(std::pmr::get_default_resource(), ring_size);

        ring.try_push(0);
        ring.try_push(0);
        ring.try_push(0);

        REQUIRE_FALSE(ring.empty());

        ring.clear();

        REQUIRE(ring.empty());
    }
}

TEST_SUITE("Ringbuffer - 1P1C - Multithread")
{
    using taskete::detail::lockfree_ringbuffer;
    constexpr std::uint32_t ring_size = 32;

    // Produce 2x the ringbuffer size
    // Consume 1x the ringbuffer size
    TEST_CASE("Produce more than what we Consume")
    {
        lockfree_ringbuffer<int> ring(std::pmr::get_default_resource(), ring_size);

        std::atomic_flag wait_flag = ATOMIC_FLAG_INIT;

        std::thread producer{ [&ring, &wait_flag]()
        {
            while (wait_flag.test_and_set(std::memory_order_acquire));

            for(std::uint32_t p = 0; p < ring_size; ++p)
            {
                CAPTURE(p);
                while (!ring.try_push(p));
            }

            INFO("Filling up the ringbuffer...");

            for (std::uint32_t p = 0; p < ring_size; ++p)
            {
                CAPTURE(p);
                while (!ring.try_push(p));
            }

            REQUIRE_FALSE(ring.try_push(0));
        } };

        INFO("Test starts.");
        wait_flag.clear(std::memory_order_release);

        for (std::uint32_t c = 0; c < ring_size; ++c)
        {
            int elem = -1;
            CAPTURE(c);
            while (!ring.try_pull(elem));
            REQUIRE(elem == c);
        }

        producer.join();
    }

    // Produce 0.5x the ringbuffer size
    // Consume 1x the ringbuffer size
    TEST_CASE("Consume more than what we produce")
    {
        lockfree_ringbuffer<int> ring(std::pmr::get_default_resource(), ring_size);

        std::atomic_flag wait_flag = ATOMIC_FLAG_INIT;

        std::thread producer{ [&ring, &wait_flag]
        {
            while (wait_flag.test_and_set(std::memory_order_acquire));

            for (std::uint32_t p = 0; p < ring_size / 2; ++p)
            {
                CAPTURE(p);
                while (!ring.try_push(p));
            }
        } };

        INFO("Test starts.");
        wait_flag.clear(std::memory_order_release);

        for (std::uint32_t c = 0; c < ring_size / 2; ++c)
        {
            int elem = -1;
            CAPTURE(c);
            while (!ring.try_pull(elem));
            REQUIRE(elem == c);
        }

        int n;
        REQUIRE_FALSE(ring.try_pull(n));

        producer.join();
    }

    // Produce/Consume with equal parts
    TEST_CASE("Balanced producer/consumer")
    {
        lockfree_ringbuffer<int> ring{ std::pmr::get_default_resource(), ring_size };

        std::atomic_flag wait_flag = ATOMIC_FLAG_INIT;

        std::thread producer{ [&ring, &wait_flag]
        {
            while (wait_flag.test_and_set(std::memory_order_acquire));
            auto sz = ring_size;
            while (sz--)
            {
                CAPTURE(sz);
                while (!ring.try_push(sz));
            }
        } };

        INFO("Test starts");
        wait_flag.clear(std::memory_order_release);

        auto sz = ring_size;
        while (sz--)
        {
            int elem = -1;
            CAPTURE(sz);
            while (!ring.try_pull(elem));
            REQUIRE(elem == sz);
        }

        producer.join();
    }
}
