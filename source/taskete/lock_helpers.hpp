#pragma once

#include <atomic>

namespace taskete::detail
{
    class spinlock
    {
    public:
        void lock() noexcept { while(_lock.test_and_set(std::memory_order_acquire)){} }
        void unlock() noexcept { _lock.clear(std::memory_order_release); }

    private:
        std::atomic_flag _lock = ATOMIC_FLAG_INIT;
    };
}