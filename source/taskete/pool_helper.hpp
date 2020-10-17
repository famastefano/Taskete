#pragma once

//#include <taskete/handle.hpp>
//#include <taskete/pool_options.hpp>

#include "../include/taskete/handle.hpp"
#include "../include/taskete/pool_options.hpp"

#include <cstdint>

namespace taskete::detail
{
    class pool_helper
    {
    private:
        std::uint32_t offset_mask;
        std::uint32_t pool_mask;
        std::uint32_t pool_shift;

        constexpr std::uint32_t log2(std::uint32_t v) const noexcept;

    public:
        pool_helper(pool_options options) noexcept;

        constexpr std::uint32_t extract_pool(handle_t handle) const noexcept;

        constexpr std::uint32_t extract_offset(handle_t handle) const noexcept;

        template<typename T>
        constexpr handle_t make_handle(std::uint32_t index, T* base, T* obj) const noexcept;
    };

    /*
    
    n :         0000'1000
    m :         0000'0111
    pm normal : 1111'1000
    
    bound     : 0001'0000
    sbound    : log2(bound)
    pm bounded: pm normal << bound >> bound;

    */

    inline pool_helper::pool_helper(pool_options options) noexcept
    {
        offset_mask = options.pool_capacity - 1;
        
        pool_shift = this->log2(offset_mask);

        pool_mask = ~offset_mask;

        if (options.max_pools) // not all bits will be used for the pool
        {
            auto bound_shift = this->log2(options.max_pools);
            pool_mask <<= bound_shift; // clears upper bits
            pool_mask >>= bound_shift;
        }
    }

    inline constexpr std::uint32_t pool_helper::extract_pool(handle_t handle) const noexcept
    {
        return (handle & pool_mask) >> pool_shift;
    }

    inline constexpr std::uint32_t pool_helper::extract_offset(handle_t handle) const noexcept
    {
        return handle & offset_mask;
    }

    template<typename T>
    inline constexpr handle_t pool_helper::make_handle(std::uint32_t index, T* base, T* obj) const noexcept
    {
        return handle_t((std::uint64_t(index) << pool_shift) | obj - base);
    }

    inline constexpr std::uint32_t pool_helper::log2(std::uint32_t v) const noexcept
    {
        if (!v)
            return 0;

        std::uint32_t r = 1; // r will be lg(v)

        while (v >>= 1)
        {
            ++r;
        }

        return r;
    }
}