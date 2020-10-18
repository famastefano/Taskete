#pragma once

#include <cstdint>
#include <memory_resource>

namespace taskete
{
    struct pool_options
    {
        // How many objects each pool can hold
        std::uint32_t pool_capacity;
        // Maximum amount of pools to use
        std::uint32_t max_pools = std::uint32_t(-1);
        // Which resource will be used to manage the pool's memory
        std::pmr::memory_resource* resource;
    };
}