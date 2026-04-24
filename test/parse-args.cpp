#include <gtest/gtest.h>

#include "config.h"

#include <nk_test_printer.h>

#include "util.h"

#define TEST_PARSE(ARG, TYPE)                                                  \
    parsed_arg arg;                                                            \
    {                                                                          \
        char input[] = ARG;                                                    \
        parse_arg(input, &arg);                                                \
        ASSERT_EQ(arg.type, TYPE);                                             \
    }
#define H(x) (x > GITNV_MAX_CACHE_NUMBER ? GITNV_MAX_CACHE_NUMBER : x)

TEST(ParseArgs, ZeroNoOp) { TEST_PARSE("0", NO_OP); }
TEST(ParseArgs, StartWithNumberNoOp) { TEST_PARSE("2bsd0", NO_OP); }
TEST(ParseArgs, InvertedRangeNoOp) { TEST_PARSE("6..5", NO_OP); }

TEST(ParseArgs, Single3) {
    TEST_PARSE("3", SINGLE);
    ASSERT_EQ(arg.val.single, 3);
}

TEST(ParseArgs, Single31) {
    TEST_PARSE("31", SINGLE);
    ASSERT_EQ(arg.val.single, 31);
}

TEST(ParseArgs, Range2_7) {
    TEST_PARSE("2..7", RANGE);
    ASSERT_EQ(arg.val.range[0], 2);
    ASSERT_EQ(arg.val.range[1], H(7));
}

TEST(ParseArgs, Range3_11) {
    TEST_PARSE("3..11", RANGE);
    ASSERT_EQ(arg.val.range[0], 3);
    ASSERT_EQ(arg.val.range[1], H(11));
}

TEST(ParseArgs, Range6_6) {
    TEST_PARSE("6..6", SINGLE);
    ASSERT_EQ(arg.val.range[0], 6);
    ASSERT_EQ(arg.val.range[1], H(6));
}

TEST(ParseArgs, Range15_100) {
    TEST_PARSE("15..100", RANGE);
    ASSERT_EQ(arg.val.range[0], 15);
    ASSERT_EQ(arg.val.range[1], H(100));
}

#undef TEST_PARSE
#undef H
