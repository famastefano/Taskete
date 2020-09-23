#pragma once

#include <vector>
#include <type_traits>
#include <cstdint>
#include <shared_mutex>
#include <algorithm>

namespace taskete::detail
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
        constexpr DestructorImplementation(std::pmr::memory_resource* res, T* t) noexcept
            : res(res), obj(t)
        {}

        void operator()() noexcept override
        {
            if constexpr (!std::is_trivially_destructible_v<T>)
                obj->~T();

            res->deallocate(obj, sizeof(T), alignof(T));
        }
    };

    struct MetaData
    {
        uint64_t key{};
        void* object = nullptr;
        TypeErasedDestructor* dtor = nullptr;

        // This avoids an additional memory allocation
        alignas(void*) unsigned char dtor_mem[sizeof(void*) * 2] = {};

        template<typename T, typename... Args>
        constexpr void init(uint64_t key, std::pmr::memory_resource* res, Args&&... args)
        {
            static_assert(sizeof(void*) * 2 == sizeof(DestructorImplementation<T>), "Can't save type erased destructor inside the buffer");

            this.key = key;

            object = res->allocate(sizeof(T), alignof(T));
            new(object) T(std::forward<Args>(args)...);

            dtor = new(dtor_mem) DestructorImplementation<T>(res, static_cast<T*>(object));
        }

        ~MetaData();
    };

    /*
     * Handles a type erased memory that will be shared between the nodes of their graph.
     * 
     * 
     */
    class SharedMemory
    {
    private:
        std::shared_mutex memory_lock;
        std::pmr::vector<MetaData> memory;

        template<typename T, uint64_t Key>
        T* read() const noexcept;

    public:
        SharedMemory(std::pmr::memory_resource* res);

        SharedMemory(SharedMemory const&) = delete;
        SharedMemory(SharedMemory&&) = delete;

        template<typename T, uint64_t Key>
        T* get() const noexcept;

        template<typename T, uint64_t Key, typename... Args>
        T* get_or_construct(Args&&... args);
    };

    template<typename T, uint64_t Key>
    inline T* SharedMemory::read() const noexcept
    {
        auto elem = std::find(memory.cbegin(), memory.cend(), [](MetaData const& item)
        {
            return item.key == Key;
        });

        return elem == memory.cend() ? nullptr : static_cast<T*>(elem.raw_mem);
    }

    template<typename T, uint64_t Key>
    inline T* SharedMemory::get() const noexcept
    {
        std::shared_lock reader{ memory_lock };

        return read<T, Key>();
    }
    template<typename T, uint64_t Key, typename ...Args>
    inline T* SharedMemory::get_or_construct(Args && ...args)
    {
        static_assert(!std::is_constructible_v<T, Args...>, "Can't construct the specified type with the given arguments.");

        {
            auto* elem = get<T, Key>();
            if (elem)
                return elem;
        }

        std::scoped_lock writer{ memory_lock };

        // maybe another thread already created the object
        // but we already own the lock in exclusive mode
        {
            auto* elem = read<T, Key>();
            if (elem)
                return elem;
        }

        auto* mem_res = memory.get_allocator().resource();
        
        MetaData data;
        data.init<T>(Key, mem_res, std::forward<Args>(args)...);

        T* elem = static_cast<T*>(data.object);

        memory.emplace_back(std::move(data));

        return elem;
    }
}