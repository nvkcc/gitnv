#include <gtest/gtest.h>

#include "test_printer.h"
#include "util.h"

TEST(Util, Uncolor1) {
    char x[] = "hello\x1b[33mthere\x1b[m";
    int y = uncolor(x, sizeof(x));
    EXPECT_STREQ(x, "hellothere");
    EXPECT_EQ(y, sizeof("hellothere"));
}

TEST(Util, Uncolor2) {
    char x[] = "\t\x1b[31mmodified:\tbuild.py\x1b[m\n";
    int y = uncolor(x, sizeof(x));
    EXPECT_STREQ(x, "\tmodified:\tbuild.py\n");
    EXPECT_EQ(y, sizeof("\tmodified:\tbuild.py\n"));
}

TEST(HelloTest, BasicAssertions) {
    // Expect two strings not to be equal.
    EXPECT_STRNE("hello", "world");
    // Expect equality.
    EXPECT_EQ(7 * 6, 42);
}

int main(int argc, char **argv) {
    // Override the default result printer.
    testing::TestEventListeners &listeners =
        testing::UnitTest::GetInstance()->listeners();
    delete listeners.Release(listeners.default_result_printer());
    listeners.Append(new MinimalistPrinter);

    testing::InitGoogleTest(&argc, argv);

    int exit_code = RUN_ALL_TESTS();
    if (exit_code != 0) {
        return exit_code;
    }
    return 0;
}
