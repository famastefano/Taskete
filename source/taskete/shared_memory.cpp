#include "shared_memory.hpp"

taskete::SharedMemory::SharedMemory(std::pmr::memory_resource* res)// : memory(res)
{
    (void)res;
}

taskete::detail::MetaData::~MetaData()
{
    (*dtor)();
    delete dtor;
}
