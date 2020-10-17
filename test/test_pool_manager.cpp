#include "../source/taskete/pool_manager.hpp"

#include <doctest.h>

// TODO: Test with non trivial types
// TODO: repeat tests in a multithreaded environment

TEST_SUITE("Pool Manager - Single Thread")
{
    using int_pool_t = taskete::detail::pool_manager<std::uint64_t>;

    taskete::pool_options get_default_options() noexcept
    {
        taskete::pool_options opt{};
        opt.resource = std::pmr::get_default_resource();
        opt.pool_capacity = 128;
        return opt;
    }

    TEST_CASE("Without pools limits, construction always succeed")
    {
        int_pool_t pool{ get_default_options() };

        std::uint64_t expected_x = 51, expected_y = 2539;

        auto x = pool.construct(expected_x);
        auto y = pool.construct(expected_y);

        REQUIRE(pool.get(x) == expected_x);
        REQUIRE(pool.get(y) == expected_y);
    }
}