// Smoke test: just exercises the test framework itself.
#include "test_framework.hpp"

TEST(arithmetic) {
    ASSERT_EQ(4, 2 + 2);
    ASSERT_NE(5, 2 + 2);
    ASSERT_TRUE(true);
    ASSERT_FALSE(false);
    ASSERT_NEAR(1.0, 0.999, 0.01);
}

RUN_ALL_TESTS()
