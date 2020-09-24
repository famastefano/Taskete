#pragma once

#include "execution_payload.hpp"

#include <atomic>
#include <cstdint>
#include <memory_resource>
#include <utility>

namespace taskete::detail
{
    template<typename T>
    class WaitList
    {
        int size{};
        T* handles = nullptr;

    public:
        using iterator = T*;

        WaitList(T* data, int sz, std::pmr::memory_resource* res) : size(sz), handles(static_cast<T*>(res->allocate(sizeof(T)*std::size_t(sz), alignof(T))))
        {
            for (int i = 0; i < sz; ++i)
                handles[i] = *data++;
        }

        constexpr WaitList(WaitList&& other)
        {
            std::swap(size, other.size);
            std::swap(handles, other.handles);
        }

        void destroy(std::pmr::memory_resource* res) noexcept
        {
            res->deallocate(handles, sizeof(T) * std::size_t(size), alignof(T));
        }

        constexpr iterator begin() noexcept { return handles; }
        constexpr iterator end() noexcept { return handles + size; }
    };

    class Node
    {
    private:
        int32_t const graph_id;
        std::atomic<int32_t> wait_counter;
        WaitList<int32_t> wait_list; // TODO: set proper handle type when the NodeMemoryManager will be created
        std::pmr::memory_resource* const mem_res;

    public:
        Node(int32_t graph, int32_t wait_no, int32_t* handle_list, int sz, std::pmr::memory_resource* res)
            : graph_id(graph), wait_counter(wait_no), wait_list(handle_list, sz, res), mem_res(res)
        {}

        Node(Node&& other) noexcept;

        ~Node();
    };
}