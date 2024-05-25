#include <doctest/doctest.h>

#include <tt/iseven.hpp>

TEST_SUITE("iseven")
{
    TEST_CASE("iseven")
    {
        REQUIRE_EQ(true, tt::iseven(0));

        REQUIRE_EQ(true, tt::iseven(2));
        REQUIRE_EQ(true, tt::iseven(-2));

        REQUIRE_EQ(false, tt::iseven(1));
        REQUIRE_EQ(false, tt::iseven(-1));
        REQUIRE_EQ(false, tt::iseven(3));
    }
}
