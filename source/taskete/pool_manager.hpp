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
    struct FreeList
    {
        FreeList* next = nullptr;
    };

    struct pool
    {
        std::byte* raw_mem = nullptr;
        FreeList* head = nullptr;
        spinlock mtx;

        pool() = default;

        pool(pool const&) = delete;
        pool(pool&& other) noexcept : raw_mem(std::exchange(other.raw_mem, nullptr)), head(std::exchange(other.head, nullptr))
        {}
    };

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

    public:
        constexpr pool_manager(pool_options options) noexcept;

        template<typename... Args>
        handle_t construct(Args&& ...args);

        void destroy(handle_t handle) noexcept(std::is_nothrow_destructible_v<T>);

        T& get(handle_t handle) noexcept;

        ~pool_manager();
    };

    template<typename T>
    inline std::pair<pool&, std::uint32_t> pool_manager<T>::find_or_create_pool()
    {
        {
            std::shared_lock sh_lock{ rw_mutex };
            auto it = std::find_if(std::begin(pools), std::end(pools), [](pool* p) -> bool { return p->head; });
            if (it != std::end(pools))
                return { **it, it - std::begin(pools) };
        }

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

    template<typename T>
    inline void pool_manager<T>::populate_list(pool& p) noexcept
    {
        auto count = options.pool_capacity;

        p.head = new(p.raw_mem) FreeList{};
        auto* current_node = p.head;

        for (std::uint32_t i = 1; i < count; ++i)
        {
            current_node->next = new(p.raw_mem + i * sizeof(T)) FreeList{};
            current_node = current_node->next;
        }
    }

    template<typename T>
    inline void pool_manager<T>::mark_as_free(pool & p, void * obj) noexcept
    {
        std::unique_lock lock{ p.mtx };

        FreeList* new_node = new(obj) FreeList{};

        if (!p.head)
        {
            p.head = new_node;
            return;
        }

        FreeList* node = p.head;
        while (node->next && node->next < new_node)
            node = node->next;

        new_node->next = node->next;
        node->next = new_node;
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

        FreeList* free_pos{};

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
        decltype(std::begin(pools)) pool;

        {
            std::shared_lock sh_lock{ rw_mutex };
            pool = std::begin(pools) + helper.extract_pool(handle);
        }

        std::unique_lock lock{ pool->mtx };
        T* obj = reinterpret_cast<T*>(pool->raw_mem) + helper.extract_offset(handle);
        
        if constexpr (!std::is_trivially_destructible_v<T>)
            obj->~T();

        mark_as_free(*pool, obj);
    }

    template<typename T>
    inline T& pool_manager<T>::get(handle_t handle) noexcept
    {
        decltype(std::begin(pools)) pool;

        {
            std::shared_lock sh_lock{ rw_mutex };
            pool = std::begin(pools) + helper.extract_pool(handle);
        }

        return *(reinterpret_cast<T*>((*pool)->raw_mem) + helper.extract_offset(handle));
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