#include <doctest.h>

#include "../source/taskete/shared_memory.hpp"

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

    }

    TEST_CASE("Constructing an existing object")
    {

    }

    TEST_CASE("Constructing a non-existing object with a non-default constructor")
    {

    }
}

TEST_SUITE("Shared Memory - Multiple Thread")
{
    TEST_CASE("Getting a non-existing object")
    {

    }

    TEST_CASE("Constructing a non-existing object")
    {

    }

    TEST_CASE("Getting an existing object")
    {

    }

    TEST_CASE("Constructing an existing object")
    {

    }

    TEST_CASE("Constructing a non-existing object with a non-default constructor")
    {

    }
}