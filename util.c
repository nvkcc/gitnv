#include "util.h"
#include "config.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/// Very naivey removes ANSI color codes.
int uncolor(char *b, int len) {
    int i = 0, j = 0;
    bool on = true;
    for (; i < len; ++i) {
        if (b[i] == '\x1b') {
            on = false;
        } else if (on) {
            b[j++] = b[i];
        } else if (b[i] == 'm') {
            on = true;
        }
    }
    return j;
}

/// Gets the number of arguments this this one argument expands out to.
///
/// "3"    -> 1
/// "8"    -> 1
/// "2..7" -> 6
/// "3..6" -> 4
/// "0..3" -> 1 (because 0 is an invalid index, so we treat that as a pathspec)
/// "0"    -> 1
/// "6..6" -> 1
/// "6..5" -> 1 (valid range but empty. So we'll treat that as a pathspec)
///
/// Since arg is guaranteed to come from `argv`, it is also guaranteed to be NUL
/// terminated.
int parse_args(const char *arg, uint64_t *cache_mask) {
    /// Length of `arg` without the NUL byte.
    int n = strlen(arg);
    char *dots;
    // Try to search for the ".." substring in the arg.
    if ((dots = memmem(arg, n, "..", 2)) == NULL) {
        // Either a regular pathspec, or a single number.
        n = atoi(arg);
        if (GITNV_IS_VALID_USER_INPUT_NUMBER(n)) {
            *cache_mask |= 1 << (n - 1);
        }
        return 1;
    }
    // Convert both the substrings on the left and right of the ".." to
    // integers. For that we need a NUL byte to be strategically placed.
    *dots = '\0';
    const int left = atoi(arg), right = atoi(dots + 2);
    *dots = '.';
    if (left == 0 || !GITNV_IS_VALID_USER_INPUT_NUMBER(right) || right < left) {
        // Treat the arg like a regular pathspec.
        return 1;
    }
    for (n = left; n <= right; ++n) {
        *cache_mask |= 1 << (n - 1);
    }
    return right - left + 1;
}

enum arg_type parse_arg2(char *arg, unsigned short *left,
                         unsigned short *right) {
    /// Length of `arg` without the NUL byte.
    int n = strlen(arg);
    char *dots;
    // Try to search for the ".." substring in the arg.
    if ((dots = memmem(arg, n, "..", 2)) == NULL) {
        // Either a regular pathspec, or a single number.
        n = atoi(arg);
        if (GITNV_IS_VALID_USER_INPUT_NUMBER(n)) {
            *left = n;
            return SINGLE;
        }
        return NO_OP;
    }
    // Convert both the substrings on the left and right of the ".." to
    // integers. For that we need a NUL byte to be strategically placed.
    *dots = '\0';
    *left = atoi(arg);
    *right = atoi(dots + 2);
    *dots = '.';
    if (*left == 0 || !GITNV_IS_VALID_USER_INPUT_NUMBER(*right) ||
        *right < *left) {
        return NO_OP;
    }
    return RANGE;
}
