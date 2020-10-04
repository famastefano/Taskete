#include "shared_memory.hpp"

taskete::SharedMemory::SharedMemory(std::pmr::memory_resource* res) : memory(res)
{}

taskete::detail::MetaData::MetaData(MetaData&& other) noexcept
{
    std::swap(dtor, other.dtor);
    std::swap(key, other.key);
    std::swap(object, other.object);
}

taskete::detail::MetaData& taskete::detail::MetaData::operator=(MetaData&& rhs) noexcept
{
    std::swap(dtor, rhs.dtor);
    std::swap(key, rhs.key);
    std::swap(object, rhs.object);

    return *this;
}

taskete::detail::MetaData::~MetaData()
{
    if(dtor)
    {
        (*dtor)();
        dtor->destroy();
    }
}
