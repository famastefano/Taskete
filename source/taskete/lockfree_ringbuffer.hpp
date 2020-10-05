#pragma once

#include <atomic>
#include <memory_resource>
#include <type_traits>

#ifdef _MSC_VER
#include <new>
#define TASKETE_L1CACHE_ALIGN alignas(std::hardware_destructive_interference_size)
#else
#define TASKETE_L1CACHE_ALIGN alignas(64)
#endif

namespace taskete::detail
{
    /*
     * Fixed size
     * Lockfree
     * Thread safe
     * Contiguous
     */
    template<typename T>
    class lockfree_ringbuffer
    {
        static_assert(std::is_default_constructible_v<T>
                      && std::is_trivially_destructible_v<T>
                      && std::is_copy_assignable_v<T>
                      , "lockfree_ringbuffer requires a type that is DefaultConstructible, TriviallyDestructible, and CopyAssignable");

    private:
        T* head;
        std::pmr::memory_resource* mem_res;
        std::uint32_t _size;
        // The only edge case is when C==P.
        // If it happened because C+1 == P then we read everything
        // Otherwise, we wrote in every possible space.
        std::atomic<bool> did_C_reached_P; // 'true' because 

        TASKETE_L1CACHE_ALIGN std::atomic<T*> producer_cursor;
        TASKETE_L1CACHE_ALIGN std::atomic<T*> consumer_cursor;

    public:
        lockfree_ringbuffer(std::pmr::memory_resource* res, std::uint32_t size);

        lockfree_ringbuffer(lockfree_ringbuffer const&) = delete;
        lockfree_ringbuffer(lockfree_ringbuffer&&) = delete;

        ~lockfree_ringbuffer();

        bool try_push(T const& elem) noexcept(noexcept(std::is_nothrow_copy_assignable_v<T>));
        bool try_pull(T& elem) noexcept(noexcept(std::is_nothrow_copy_assignable_v<T>));

        bool empty() noexcept;
        void clear() noexcept;

        std::uint32_t size() const noexcept;
        std::uint32_t free_space() const noexcept;
    };

    template<typename T>
    inline lockfree_ringbuffer<T>::lockfree_ringbuffer(std::pmr::memory_resource* res, std::uint32_t size)
    {
        head = static_cast<T*>(res->allocate(sizeof(T) * size, alignof(T)));
        mem_res = res;
        this->_size = size;

        T* item = head;
        while (size--)
            new(item++) T;

        producer_cursor.store(head, std::memory_order_release);
        consumer_cursor.store(head, std::memory_order_release);
        did_C_reached_P.store(true, std::memory_order_release); // 'yes we are empty!'
    }

    template<typename T>
    inline lockfree_ringbuffer<T>::~lockfree_ringbuffer()
    {
        mem_res->deallocate(head, sizeof(T) * _size, alignof(T));
    }

    template<typename T>
    inline bool lockfree_ringbuffer<T>::try_push(T const& elem) noexcept(noexcept(std::is_nothrow_copy_assignable_v<T>))
    {
        auto* producer = producer_cursor.load(std::memory_order_acquire);
        auto* consumer = consumer_cursor.load(std::memory_order_acquire);

        if (producer == consumer && !did_C_reached_P.load(std::memory_order_acquire)) // P wrote everywhere up to C
            return false;

        if (++producer == head + size())
            producer = head;

        *producer = elem;
        producer_cursor.store(producer, std::memory_order_release);
        if (producer == consumer_cursor.load(std::memory_order_acquire))
            did_C_reached_P.store(false, std::memory_order_release);

        return true;
    }

    template<typename T>
    inline bool lockfree_ringbuffer<T>::try_pull(T& elem) noexcept(noexcept(std::is_nothrow_copy_assignable_v<T>))
    {
        auto* producer = producer_cursor.load(std::memory_order_acquire);
        auto* consumer = consumer_cursor.load(std::memory_order_acquire);

        if (producer == consumer && did_C_reached_P.load(std::memory_order_acquire)) // C read everything so we can't read anymore
            return false;

        if (++consumer == head + size())
            consumer = head;

        elem = *consumer;
        consumer_cursor.store(consumer, std::memory_order_release);
        if (consumer == producer)
            did_C_reached_P.store(true, std::memory_order_release);

        return true;
    }

    template<typename T>
    inline bool lockfree_ringbuffer<T>::empty() noexcept
    {
        auto* producer = producer_cursor.load(std::memory_order_acquire);
        auto* consumer = consumer_cursor.load(std::memory_order_acquire);

        return producer == consumer && did_C_reached_P.load(std::memory_order_acquire);
    }

    template<typename T>
    inline void lockfree_ringbuffer<T>::clear() noexcept
    {
        auto* producer = producer_cursor.load(std::memory_order_acquire);
        consumer_cursor.store(producer, std::memory_order_release);
        did_C_reached_P.store(true, std::memory_order_release);
    }

    template<typename T>
    inline std::uint32_t lockfree_ringbuffer<T>::size() const noexcept
    {
        return _size;
    }

    template<typename T>
    inline std::uint32_t lockfree_ringbuffer<T>::free_space() const noexcept
    {
        auto* producer = producer_cursor.load(std::memory_order_acquire);
        auto* consumer = consumer_cursor.load(std::memory_order_acquire);

        if(producer == consumer)
            return did_C_reached_P.load(std::memory_order_acquire) ? 0 : size();
            
        if (producer < consumer)
            return consumer - producer;

        return size() - (consumer - producer);
    }
    
}
