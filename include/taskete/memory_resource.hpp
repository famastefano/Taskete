#pragma once

#include <memory_resource>

namespace taskete
{
    std::pmr::memory_resource* get_default_logging_resource() noexcept;
}