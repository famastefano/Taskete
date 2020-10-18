#pragma once

//#include <taskete/handle.hpp>
//#include <taskete/pool_options.hpp>

#include "../include/taskete/handle.hpp"
#include "../include/taskete/pool_options.hpp"

#include "pool_helper.hpp"
#include "lock_helpers.hpp"

#include <cstddef>
#include <type_traits>
#include <vector>
#include <algorithm>
#include <shared_mutex>
#include <utility>
#include <atomic>

namespace taskete::detail
{
    struct free_list
    {
        free_list* next = nullptr;
    };

    struct pool
    {
        std::byte* raw_mem = nullptr;
        free_list* head = nullptr;
        spinlock mtx;

        pool() = default;

        pool(pool const&) = delete;
        pool(pool&& other) noexcept : raw_mem(std::exchange(other.raw_mem, nullptr)), head(std::exchange(other.head, nullptr))
        {}
    };

    /*
     * Pool Allocator that uses handles instead of raw pointers.
     */
    template<typename T>
    class pool_manager
    {
    private:
        std::pmr::vector<pool*> pools;
        pool_options options;
        pool_helper helper;
        std::shared_mutex rw_mutex;

        std::pair<pool&, std::uint32_t> find_or_create_pool();
        void populate_list(pool& p) noexcept;
        void mark_as_free(pool& p, void* obj) noexcept;
        pool& get_pool(handle_t handle) noexcept;

    public:
        constexpr pool_manager(pool_options options) noexcept;

        template<typename... Args>
        handle_t construct(Args&& ...args);

        void destroy(handle_t handle) noexcept(std::is_nothrow_destructible_v<T>);

        T& get(handle_t handle) noexcept;

        ~pool_manager();
    };

    /*
     * Returns a pool with a free block.
     * It tries to find an existing pool, otherwise it constructs a new one.
     * 
     * Throws: bad_alloc
     *         when it can't allocate more pools
     */
    template<typename T>
    inline std::pair<pool&, std::uint32_t> pool_manager<T>::find_or_create_pool()
    {
        {
            std::shared_lock sh_lock{ rw_mutex };
            auto it = std::find_if(std::begin(pools), std::end(pools), [](pool* p) -> bool { return p->head; });
            if (it != std::end(pools))
                return { **it, it - std::begin(pools) };

            if (pools.size() == options.max_pools)
                throw std::bad_alloc{};
        }

        // Construct the new pool

        auto * p = new taskete::detail::pool();
        auto * resource = pools.get_allocator().resource();
        p->raw_mem = static_cast<std::byte*>(resource->allocate(options.pool_capacity * sizeof(T), alignof(T)));
        populate_list(*p);

        {
            std::unique_lock ex_lock{ rw_mutex };
            pools.emplace_back(std::move(p));
            auto it = --std::end(pools);
            return { **it, it - std::begin(pools) };
        }
    }
    
    /*
     * Initializes the pool's free list.
     */
    template<typename T>
    inline void pool_manager<T>::populate_list(pool& p) noexcept
    {
        auto count = options.pool_capacity;

        p.head = new(p.raw_mem) free_list{};
        auto* current_node = p.head;

        for (std::uint32_t i = 1; i < count; ++i) // 1st node already created
        {
            current_node->next = new(p.raw_mem + i * sizeof(T)) free_list{};
            current_node = current_node->next;
        }
    }

    /*
     * Marks a block as free by adding it to the free list.
     */
    template<typename T>
    inline void pool_manager<T>::mark_as_free(pool & p, void * obj) noexcept
    {
        std::unique_lock lock{ p.mtx };

        free_list* new_node = new(obj) free_list{};

        if (!p.head) // we are the new head
        {
            p.head = new_node;
            return;
        }

        // We keep the free list ordered by address.

        free_list* node = p.head;
        while (node->next && node->next < new_node)
            node = node->next;

        new_node->next = node->next;
        node->next = new_node;
    }

    template<typename T>
    inline pool& pool_manager<T>::get_pool(handle_t handle) noexcept
    {
        std::shared_lock sh_lock{ rw_mutex };
        auto pool = std::begin(pools) + helper.extract_pool(handle);
        return **pool;
    }

    template<typename T>
    inline constexpr pool_manager<T>::pool_manager(pool_options options) noexcept : pools(options.resource), options(options), helper(options)
    {
    }

    template<typename T>
    template<typename ...Args>
    inline handle_t pool_manager<T>::construct(Args && ...args)
    {
        static_assert(std::is_constructible_v<T, Args...>, "Can't construct the object with the given arguments.");

        auto [pool, index] = find_or_create_pool();

        free_list* free_pos{};

        {
            std::unique_lock lock{ pool.mtx };
            free_pos = pool.head;
            pool.head = pool.head->next;
        }

        return helper.make_handle(index, reinterpret_cast<T*>(pool.raw_mem), new(free_pos) T{ std::forward<Args>(args)... });
    }

    template<typename T>
    inline void pool_manager<T>::destroy(handle_t handle) noexcept(std::is_nothrow_destructible_v<T>)
    {
        auto& pool = get_pool(handle);

        std::unique_lock lock{ pool.mtx };
        T* obj = reinterpret_cast<T*>(pool.raw_mem) + helper.extract_offset(handle);
        
        if constexpr (!std::is_trivially_destructible_v<T>)
            obj->~T();

        mark_as_free(pool, obj);
    }

    template<typename T>
    inline T& pool_manager<T>::get(handle_t handle) noexcept
    {
        auto& pool = get_pool(handle);

        return *(reinterpret_cast<T*>(pool.raw_mem) + helper.extract_offset(handle));
    }

    template<typename T>
    inline pool_manager<T>::~pool_manager()
    {
        auto* resource = pools.get_allocator().resource();
        for (auto* p : pools)
        {
            resource->deallocate(p->raw_mem, options.pool_capacity * sizeof(T), alignof(T));
            delete p;
        }
    }
}