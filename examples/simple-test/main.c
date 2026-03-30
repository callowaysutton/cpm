#define SHT_IMPLEMENTATION
#include "sht.h"

#include <stdio.h>
#include <string.h>

TEST(SimpleTests, Addition) {
    EXPECT_EQ(2 + 2, 4);
}

TEST(SimpleTests, StringComparison) {
    const char *str = "hello";
    EXPECT_STR_EQ(str, "hello");
}

TEST(SimpleTests, AlwaysPassing) {
    EXPECT_TRUE(1 == 1);
}

TEST_RUN_MAIN();