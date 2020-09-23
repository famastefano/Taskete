#include "shared_memory.hpp"

taskete::detail::SharedMemory::SharedMemory(std::pmr::memory_resource* res) : memory(res)
{}

taskete::detail::MetaData::~MetaData()
{
    (*dtor)();
    delete dtor;
}
