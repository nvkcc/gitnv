#include <gtest/gtest.h>

#include "config.h"

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

TEST(ParseArgs, Single1) {
    parsed_arg arg;
#define TEST_PARSE(ARG, TYPE)                                                  \
    {                                                                          \
        char input[] = ARG;                                                    \
        parse_arg(input, &arg);                                                \
        ASSERT_EQ(arg.type, TYPE);                                             \
    }
#define H(x) (x > GITNV_MAX_CACHE_NUMBER ? GITNV_MAX_CACHE_NUMBER : x)
    TEST_PARSE("3", SINGLE);
    ASSERT_EQ(arg.val.single, 3);
    TEST_PARSE("8", SINGLE);
    ASSERT_EQ(arg.val.single, 8);
    TEST_PARSE("0", NO_OP);
    TEST_PARSE("2..7", RANGE);
    ASSERT_EQ(arg.val.range[0], 2);
    ASSERT_EQ(arg.val.range[1], H(7));
    TEST_PARSE("3..11", RANGE);
    ASSERT_EQ(arg.val.range[0], 3);
    ASSERT_EQ(arg.val.range[1], H(11));
    TEST_PARSE("6..6", RANGE);
    ASSERT_EQ(arg.val.range[0], 6);
    ASSERT_EQ(arg.val.range[1], H(6));
    TEST_PARSE("6..5", NO_OP);
    TEST_PARSE("2beaaed", NO_OP);
    TEST_PARSE("15..25", RANGE);
    ASSERT_EQ(arg.val.range[0], 15);
    ASSERT_EQ(arg.val.range[1], H(25));
#undef TEST_PARSE
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
