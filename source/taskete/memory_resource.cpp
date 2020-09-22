#include <taskete/memory_resource.hpp>

#ifdef TASKETE_HAS_SPDLOG
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#else
#include <cstdio> // fallback to fprintf
#include <atomic> // spinlock
#endif

// std::aligned_alloc is not implemented in VS2019
// see also: https://stackoverflow.com/questions/62962839/stdaligned-alloc-missing-from-visual-studio-2019c
#ifndef _MSC_VER
#include <cstdlib>
#endif

namespace taskete::detail
{
    /// <summary>
    /// Manages a thread-safe logger that writes to a file.
    /// If spdlog is not used, it will fallback to fprintf.
    /// </summary>
    class Logger
    {
        #ifdef TASKETE_HAS_SPDLOG
        std::shared_ptr<spdlog::logger> logger;
        #else
        std::FILE* fd;
        std::atomic_flag spinlock = ATOMIC_FLAG_INIT;
        #endif
    public:
        Logger([[maybe_unused]] char const* name, char const* filename) noexcept
        {
            #ifdef TASKETE_HAS_SPDLOG
            logger = spdlog::basic_logger_mt(name, filename);
            logger->set_pattern("[TH: %t] %v");
            #else
            fd = std::fopen(filename, "w+");
            #endif
        }

        template<typename... Args>
        inline void log(Args&&... args)
        {
            // "[TH: thread id] ALLOCATED 123 bytes at 0x123 with 123 alignment"

            #ifdef TASKETE_HAS_SPDLOG
            logger->log(spdlog::level::debug, "{:>12} {} bytes at {} with {} alignment", std::forward<Args>(args)...);
            #else
            while (spinlock.test_and_set(std::memory_order_acquire)) {}

            fprintf(fd, "%-12s %lu bytes at %p with %lu alignment\n", std::forward<Args>(args)...);

            spinlock.clear(std::memory_order::memory_order_release);
            #endif
        }

        #ifndef TASKETE_HAS_SPDLOG
        ~Logger()
        {
            std::fclose(fd);
        }
        #endif
    };

    class logging_resource final : public std::pmr::memory_resource
    {
        virtual void* do_allocate(std::size_t bytes, std::size_t alignment) override;
        virtual void do_deallocate(void* p, std::size_t bytes, std::size_t alignment) override;
        virtual bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override;

        Logger logger{ "new/delete logger", "new_delete_log.txt" };
    };

    void* logging_resource::do_allocate(std::size_t bytes, std::size_t alignment)
    {
        #ifndef _MSC_VER // Clang
        void* p = std::aligned_alloc(bytes, alignment);
        #else // MSVC
        void* p = _aligned_malloc(bytes, alignment);
        #endif

        // "[TH: thread id] ALLOCATED 123 bytes at 0x123 with 123 alignment"

        logger.log("ALLOCATED", static_cast<unsigned long>(bytes), p, static_cast<unsigned long>(alignment));

        return p;
    }

    void logging_resource::do_deallocate(void* p, std::size_t bytes, std::size_t alignment)
    {
        logger.log("DEALLOCATED", static_cast<unsigned long>(bytes), p, static_cast<unsigned long>(alignment));

        #ifndef _MSC_VER // Clang
        std::free(p);
        #else // MSVC
        _aligned_free(p);
        #endif
    }

    bool logging_resource::do_is_equal(const std::pmr::memory_resource & other) const noexcept
    {
        return std::addressof(other) == std::addressof(*this);
    }
}

std::pmr::memory_resource* taskete::get_default_logging_resource() noexcept
{
    static detail::logging_resource _r{};

    return &_r;
}
