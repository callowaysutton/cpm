#define SHT_IMPLEMENTATION
#include "liba.h"
#include "sht.h"
#include <stdio.h>

TEST(AppTest, SimpleAssertions) {
    int sum = liba_add(5, 3);
    EXPECT_EQ(sum, 8);
    int product = liba_multiply(4, 6);
    EXPECT_EQ(product, 24);
    EXPECT_TRUE(sum + product == 32);
}

TEST_RUN_MAIN();
