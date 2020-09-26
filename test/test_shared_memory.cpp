#include <doctest.h>

#include "../source/taskete/shared_memory.hpp"

// TODO: constructing complex objects
// TODO: constructing objects with complex destructors
// TODO: reading/writing to multiple objects

namespace
{
    template<typename Callable>
    void setup(std::vector<std::thread>& pool, Callable&& c) noexcept
    {
        int i = 0;
        for (auto& th : pool)
            th = std::move(std::thread{ std::forward<Callable>(c), i++ });
    }

    void wait_all(std::vector<std::thread>& pool) noexcept
    {
        for (auto& th : pool)
            if (th.joinable())
                th.join();
    }
}

TEST_SUITE("Shared Memory - Single Thread")
{
    TEST_CASE("Getting a non-existing object")
    {
        taskete::SharedMemory shmem{ std::pmr::get_default_resource() };

        REQUIRE(shmem.get<int>(0) == nullptr);
    }

    TEST_CASE("Constructing a non-existing object")
    {
        taskete::SharedMemory shmem{ std::pmr::get_default_resource() };

        int* data = nullptr;
        data = shmem.get_or_construct<int>(0);
        REQUIRE(data != nullptr);
        REQUIRE(*data == int());

        int* initialized_data = nullptr;
        initialized_data = shmem.get_or_construct<int>(1423, 789);
        REQUIRE(initialized_data != nullptr);
        REQUIRE(*initialized_data == 789);
    }

    TEST_CASE("Getting an existing object")
    {
        taskete::SharedMemory shmem{ std::pmr::get_default_resource() };

        int* expected = shmem.get_or_construct<int>(91, 412);
        REQUIRE(expected != nullptr);

        int* actual = shmem.get<int>(91);
        REQUIRE(actual == expected);
        REQUIRE(*actual == *expected);
    }

    TEST_CASE("Constructing an existing object")
    {
        taskete::SharedMemory shmem{ std::pmr::get_default_resource() };

        int expected = 21;
        int* obj = shmem.get_or_construct<int>(0, expected);
        int* same_obj = shmem.get_or_construct<int>(0); // default ctor

        REQUIRE(obj == same_obj);
        REQUIRE(*obj == expected);
        REQUIRE(*same_obj == expected);
    }
}

TEST_SUITE("Shared Memory - Multiple Thread")
{
    std::vector<std::thread> pool(20);
    std::vector<int*> data(20);

    TEST_CASE("Getting a non-existing object")
    {
        taskete::SharedMemory shmem{ std::pmr::get_default_resource() };

        setup(pool, [&shmem](int i) { data[i] = shmem.get<int>(0); });
        wait_all(pool);

        for (auto d : data)
            REQUIRE(d == nullptr);
    }

    TEST_CASE("Constructing a non-existing object")
    {
        taskete::SharedMemory shmem{ std::pmr::get_default_resource() };
        int expected = 41235;

        setup(pool, [&shmem, expected](int i) { data[i] = shmem.get_or_construct<int>(0, expected); });
        wait_all(pool);

        for (auto d : data)
            REQUIRE(d != nullptr);

        for (auto d : data)
            REQUIRE(*d == expected);

        int* p = data[0];
        for (auto d : data)
        {
            REQUIRE(d == p);
            p = d;
        }
    }

    TEST_CASE("Getting an existing object")
    {
        taskete::SharedMemory shmem{ std::pmr::get_default_resource() };

        int expected = 124;
        int* obj = shmem.get_or_construct<int>(0, expected);

        setup(pool, [&shmem](int i) { data[i] = shmem.get<int>(0); });
        wait_all(pool);

        for (auto d : data)
        {
            REQUIRE(d == obj);
            REQUIRE(*d == *obj);
        }
    }

    TEST_CASE("Constructing an existing object")
    {
        taskete::SharedMemory shmem{ std::pmr::get_default_resource() };

        int expected = 987;
        int* obj = shmem.get_or_construct<int>(97, expected);

        setup(pool, [&shmem](int i) { data[i] = shmem.get_or_construct<int>(97, i); });
        wait_all(pool);

        for (auto d : data)
        {
            REQUIRE(d == obj);
            REQUIRE(*d == *obj);
        }
    }
}
