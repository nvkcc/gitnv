#include <gtest/gtest.h>

#include <nk_test_printer.h>

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

TEST(Util, ParseArgs) {
#define TEST_PARSE(ARG, NUM_ARGS, MASK)                                        \
    {                                                                          \
        char arg[] = ARG;                                                      \
        uint64_t __mask = 0;                                                   \
        ASSERT_EQ(parse_args(arg, &__mask), NUM_ARGS);                         \
        ASSERT_EQ(__mask, MASK);                                               \
    }
    TEST_PARSE("3", 1, 0b100);
    TEST_PARSE("8", 1, 0b10000000);
    TEST_PARSE("0", 1, 0);
    TEST_PARSE("2..7", 6, 0b1111110);
    TEST_PARSE("3..11", 9, 0b11111111100);
    TEST_PARSE("6..6", 1, 0b100000);
    TEST_PARSE("6..5", 1, 0);
#undef TEST_PARSE
}

TEST(Util, ParseArgs2) {
    unsigned short left, right;
#define TEST_PARSE(ARG, OUTCOME)                                               \
    {                                                                          \
        char arg[] = ARG;                                                      \
        ASSERT_EQ(parse_arg2(arg, &left, &right), OUTCOME);                    \
    }
    TEST_PARSE("3", SINGLE);
    ASSERT_EQ(left, 3);
    TEST_PARSE("8", SINGLE);
    ASSERT_EQ(left, 8);
    TEST_PARSE("0", NO_OP);
    TEST_PARSE("2..7", RANGE);
    ASSERT_EQ(left, 2);
    ASSERT_EQ(right, 7);
    TEST_PARSE("3..11", RANGE);
    ASSERT_EQ(left, 3);
    ASSERT_EQ(right, 11);
    TEST_PARSE("6..6", RANGE);
    ASSERT_EQ(left, 6);
    ASSERT_EQ(right, 6);
    TEST_PARSE("6..5", NO_OP);
#undef TEST_PARSE
}

int main(int argc, char **argv) {
    // Override the default result printer.
    testing::TestEventListeners &listeners =
        testing::UnitTest::GetInstance()->listeners();
    delete listeners.Release(listeners.default_result_printer());
    listeners.Append(new NkTestPrinter);

    testing::InitGoogleTest(&argc, argv);

    int exit_code = RUN_ALL_TESTS();
    if (exit_code != 0) {
        return exit_code;
    }
    return 0;
}
