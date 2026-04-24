#include <gtest/gtest.h>

#include <nk_test_printer.h>

#include "util.h"

TEST(Uncolor, Case1) {
    char x[] = "hello\x1b[33mthere\x1b[m";
    int y = uncolor(x, sizeof(x));
    EXPECT_STREQ(x, "hellothere");
    EXPECT_EQ(y, sizeof("hellothere"));
}

TEST(Uncolor, Case2) {
    char x[] = "\t\x1b[31mmodified:\tbuild.py\x1b[m\n";
    int y = uncolor(x, sizeof(x));
    EXPECT_STREQ(x, "\tmodified:\tbuild.py\n");
    EXPECT_EQ(y, sizeof("\tmodified:\tbuild.py\n"));
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);

    // Override the default result printer.
    ::testing::TestEventListeners &listeners =
        ::testing::UnitTest::GetInstance()->listeners();
    delete listeners.Release(listeners.default_result_printer());
    listeners.Append(new NkTestPrinter);

    return RUN_ALL_TESTS();
}
