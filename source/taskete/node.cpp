#include "node.hpp"

taskete::detail::Node::Node(Node&& other) noexcept
    : graph_id(other.graph_id),
    wait_counter(other.wait_counter.load(std::memory_order_relaxed)),
    wait_list(std::move(other.wait_list)),
    mem_res(other.mem_res)
{}

taskete::detail::Node::~Node()
{
    wait_list.destroy(mem_res);
}
