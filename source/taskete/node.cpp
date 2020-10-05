#include "node.hpp"

taskete::detail::node::node(node&& other) noexcept
    : graph_id(other.graph_id)
    , wait_counter(other.wait_counter.load(std::memory_order_acquire))
    , exec_payload(other.exec_payload)
    , wait_list(std::move(other.wait_list))
{
    other.exec_payload = nullptr;
}

void taskete::detail::node::destroy(std::pmr::memory_resource* res) noexcept
{
    exec_payload->~execution_payload();
    res->deallocate(exec_payload, exec_payload->size_of());
    wait_list.destroy(res);
}
