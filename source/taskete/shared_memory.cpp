#include "shared_memory.hpp"

taskete::shared_memory::shared_memory(std::pmr::memory_resource* res) : memory(res)
{}

taskete::detail::meta_data::meta_data(meta_data&& other) noexcept
{
    std::swap(dtor, other.dtor);
    std::swap(key, other.key);
    std::swap(object, other.object);
}

taskete::detail::meta_data& taskete::detail::meta_data::operator=(meta_data&& rhs) noexcept
{
    std::swap(dtor, rhs.dtor);
    std::swap(key, rhs.key);
    std::swap(object, rhs.object);

    return *this;
}

taskete::detail::meta_data::~meta_data()
{
    if(dtor)
    {
        (*dtor)();
        dtor->destroy();
    }
}
