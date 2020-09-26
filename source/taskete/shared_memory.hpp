#pragma once

#include "macro_utils.hpp"

#include <vector>
#include <type_traits>
#include <cstdint>
#include <shared_mutex>
#include <algorithm>
#include <memory_resource>
#include <utility>

namespace taskete
{
    namespace detail
    {
        class TypeErasedDestructor
        {
        public:
            virtual void operator()() noexcept = 0;
            virtual ~TypeErasedDestructor()
            {}
        };

        /*
         * Our SharedMemory doesn't store the object's types,
         * but it's still responsible of their cleanup.
         *
         * This class does it for us:
         * 1. calls the object destructor
         * 2. deallocates the object
         */
        template<typename T>
        class DestructorImplementation final : public virtual TypeErasedDestructor
        {
        private:
            std::pmr::memory_resource* res;
            T* obj;

        public:
            DestructorImplementation(std::pmr::memory_resource* res, T* t) noexcept
                : res(res), obj(t)
            {}

            void operator()() noexcept override
            {
                if constexpr (!std::is_trivially_destructible_v<T>)
                    obj->~T();

                res->deallocate(obj, sizeof(T), alignof(T));
            }
        };

        class TASKETE_LIB_SYMBOLS MetaData
        {
        private:
            TypeErasedDestructor* dtor = nullptr;

        public:
            uint64_t key{};
            void* object = nullptr;

            MetaData() = default;

            MetaData(MetaData&& other) noexcept
            {
                std::swap(dtor, other.dtor);
                std::swap(key, other.key);
                std::swap(object, other.object);
            }

            MetaData& operator=(MetaData&& rhs) noexcept
            {
                std::swap(dtor, rhs.dtor);
                std::swap(key, rhs.key);
                std::swap(object, rhs.object);

                return *this;
            }
        
            template<typename T, typename... Args>
            static MetaData create(uint64_t _key, std::pmr::memory_resource* res, Args&&... args)
            {
                MetaData data{};

                data.key = _key;

                data.object = res->allocate(sizeof(T), alignof(T));
                T* obj_ptr = new(data.object) T{ std::forward<Args>(args)... };

                void* dtor_mem = res->allocate(sizeof(DestructorImplementation<T>));

                data.dtor = new(dtor_mem) DestructorImplementation<T>(res, obj_ptr);

                return std::move(data);
            }
        
            ~MetaData();
        };


    } // namespace detail

    /*
     * Handles a type erased memory that will be shared between the nodes of their graph.
     * 
     * 
     */
    class TASKETE_LIB_SYMBOLS SharedMemory
    {
    private:
        mutable std::shared_mutex memory_lock;
        std::pmr::vector<detail::MetaData> memory;

        template<typename T>
        T* read(uint64_t key) const noexcept;

    public:
        SharedMemory(std::pmr::memory_resource* res);

        SharedMemory(SharedMemory const&) = delete;
        SharedMemory(SharedMemory&&) = delete;

        template<typename T>
        [[nodiscard]] T* get(uint64_t key) const noexcept;

        template<typename T, typename... Args>
        [[nodiscard]] T* get_or_construct(uint64_t key, Args&&... args);
    };

    template<typename T>
    inline T* SharedMemory::read(uint64_t key) const noexcept
    {
        auto elem = std::find_if(memory.cbegin(), memory.cend(), [key](detail::MetaData const& item)
        {
            return item.key == key;
        });

        return elem == memory.cend() ? nullptr : static_cast<T*>(elem->object);
    }

    template<typename T>
    inline T* SharedMemory::get(uint64_t key) const noexcept
    {
        std::shared_lock reader{ memory_lock };

        return read<T>(key);
    }
    template<typename T, typename ...Args>
    inline T* SharedMemory::get_or_construct(uint64_t key, Args && ...args)
    {
        static_assert(!std::is_reference_v<T>, "Reference types are not allowed.");

        if constexpr (sizeof...(Args))
            static_assert(std::is_constructible_v<T, Args...>, "Can't construct the specified type with the given arguments.");
        else
            static_assert(std::is_default_constructible_v<T>, "Can't default construct the specified type.");

        {
            auto* elem = get<T>(key);
            if (elem)
                return elem;
        }

        std::unique_lock writer{ memory_lock };

        // maybe another thread already created the object
        {
            auto* elem = read<T>(key);
            if (elem)
                return elem;
        }

        auto* mem_res = memory.get_allocator().resource();
        
        auto data = detail::MetaData::create<T>(key, mem_res, std::forward<Args>(args)...);

        T* elem = static_cast<T*>(data.object);

        memory.emplace_back(std::move(data));

        return elem;
    }
}
