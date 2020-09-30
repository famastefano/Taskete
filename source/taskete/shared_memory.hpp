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
        };

        /*
         * SharedMemory doesn't store the object's types,
         * but it's still responsible cleanup them.
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

            MetaData(MetaData&& other) noexcept;
            MetaData& operator=(MetaData&& rhs) noexcept;

            // MetaData factory
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

    /// <summary>
    /// Thread-safe associative container of heterogeneous types.
    /// 
    /// Accessing the object through its pointer is not thread-safe.
    /// </summary>
    class TASKETE_LIB_SYMBOLS SharedMemory
    {
    private:
        std::pmr::vector<detail::MetaData> memory;
        mutable std::shared_mutex memory_lock;

        /*
         * Finds an object or returns nullptr.
         * Doesn't lock!
         */
        template<typename T>
        T* read(uint64_t key) const noexcept;

    public:
        SharedMemory(std::pmr::memory_resource* res);

        SharedMemory(SharedMemory const&) = delete;
        SharedMemory(SharedMemory&&) = delete;

        /// <summary>
        /// Looks for the specified object and converts it to the provided type.
        /// Undefined behaviour if the existing object can't be accessed through T*.
        /// </summary>
        /// <typeparam name="T">Object's Type.</typeparam>
        /// <param name="key">Object's Key</param>
        /// <returns>A pointer to the object, or nullptr otherwise.</returns>
        template<typename T>
        [[nodiscard]] T* get(uint64_t key) const noexcept;

        /// <summary>
        /// Looks for the specified object, and constructs it if it doesn't exist yet.
        /// Undefined behaviour if the existing object can't be accessed through T*.
        /// 
        /// If no constructor's arguments are provided, the object must meet the DefaultConstructible requirements.
        /// 
        /// Object's construction is atomic but unordered, so only 1 thread
        /// will construct the object but it's not guaranteed which one will.
        /// </summary>
        /// <typeparam name="T">Object's Type.</typeparam>
        /// <typeparam name="...Args">Constructor's Types.</typeparam>
        /// <param name="key">Object's Key</param>
        /// <param name="...args">Constructor's arguments.</param>
        /// <returns>A pointer to the object.</returns>
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
            static_assert(std::is_constructible_v<T, Args...>, "Couldn't find a constructor that matches the provided arguments.");
        else
            static_assert(std::is_default_constructible_v<T>, "If no arguments are provided, the type must be default constructible.");

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
