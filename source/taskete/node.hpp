#pragma once

#include <taskete/handle.hpp>

#include "execution_payload.hpp"

#include <atomic>
#include <cstdint>
#include <memory_resource>
#include <utility>

namespace taskete::detail
{
    template<typename T>
    class wait_list
    {
        T* handles = nullptr;
        std::uint32_t size{};

    public:
        using iterator = T*;

        wait_list(std::pmr::memory_resource* res, T* data, std::uint32_t sz) : handles(static_cast<T*>(res->allocate(sizeof(T)* std::size_t(sz), alignof(T)))), size(sz)
        {
            for (std::uint32_t i = 0; i < sz; ++i)
                handles[i] = *data++;
        }

        wait_list(wait_list&& other) noexcept;

        void destroy(std::pmr::memory_resource* res) noexcept;

        constexpr iterator begin() noexcept { return handles; }
        constexpr iterator end() noexcept { return handles + size; }
    };

    class node
    {
    public:
        int32_t const graph_id;
        std::atomic<int32_t> wait_counter;
        execution_payload* exec_payload;
        wait_list<handle_t> wait_list;

        node(std::pmr::memory_resource* res, int32_t graph, int32_t wait_no, execution_payload* payload, handle_t* handle_list, std::uint32_t sz)
            : graph_id(graph), wait_counter(wait_no), exec_payload(payload), wait_list(res, handle_list, sz)
        {}

        node(node&& other) noexcept;

        void destroy(std::pmr::memory_resource* res) noexcept;
    };

    template<typename T>
    inline wait_list<T>::wait_list(wait_list&& other) noexcept
    {
        std::swap(handles, other.handles);
        std::swap(size, other.size);
    }
    template<typename T>
    inline void wait_list<T>::destroy(std::pmr::memory_resource* res) noexcept
    {
        res->deallocate(handles, sizeof(T) * size, alignof(T));
    }
}