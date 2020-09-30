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
    class WaitList
    {
        T* handles = nullptr;
        std::uint32_t size{};

    public:
        using iterator = T*;

        WaitList(std::pmr::memory_resource* res, T* data, std::uint32_t sz) : handles(static_cast<T*>(res->allocate(sizeof(T)* std::size_t(sz), alignof(T)))), size(sz)
        {
            for (std::uint32_t i = 0; i < sz; ++i)
                handles[i] = *data++;
        }

        WaitList(WaitList&& other) noexcept;

        void destroy(std::pmr::memory_resource* res) noexcept;

        constexpr iterator begin() noexcept { return handles; }
        constexpr iterator end() noexcept { return handles + size; }
    };

    class Node
    {
    public:
        int32_t const graph_id;
        std::atomic<int32_t> wait_counter;
        ExecutionPayload* exec_payload;
        WaitList<handle_t> wait_list;

        Node(std::pmr::memory_resource* res, int32_t graph, int32_t wait_no, ExecutionPayload* payload, handle_t* handle_list, std::uint32_t sz)
            : graph_id(graph), wait_counter(wait_no), exec_payload(payload), wait_list(res, handle_list, sz)
        {}

        Node(Node&& other) noexcept;

        void destroy(std::pmr::memory_resource* res) noexcept;
    };

    template<typename T>
    inline WaitList<T>::WaitList(WaitList&& other) noexcept
    {
        std::swap(handles, other.handles);
        std::swap(size, other.size);
    }
    template<typename T>
    inline void WaitList<T>::destroy(std::pmr::memory_resource* res) noexcept
    {
        res->deallocate(handles, sizeof(T) * size, alignof(T));
    }
}