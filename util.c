#include "util.h"
#include "config.h"
#include <ctype.h>
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

/// String to integer but ranged.
inline int antoi(char *s, int len) {
    while (--len >= 0) {
        if (!isdigit(s[len])) {
            return 0;
        }
    }
    return atoi(s);
}

void parse_arg(char *arg, parsed_arg *out) {
    /// Length of `arg` without the NUL byte.
    int n = strlen(arg);
    char *dots;
    // Try to search for the ".." substring in the arg.
    if ((dots = memmem(arg, n, "..", 2)) == NULL) {
        // Either a regular pathspec, or a single number.
        n = antoi(arg, n);
        if (GITNV_IS_VALID_USER_INPUT_NUMBER(n)) {
            out->type = SINGLE;
            out->val.single = n;
        } else {
            out->type = NO_OP;
        }
        return;
    }
    // Convert both the substrings on the left and right of the ".." to
    // integers. For that we need a NUL byte to be strategically placed.
#define L out->val.range[0]
#define R out->val.range[1]
    L = antoi(arg, dots - arg);
    R = antoi(dots + 2, arg + n - (dots + 2));
    if (L == 0 || !GITNV_IS_VALID_USER_INPUT_NUMBER(R) || R < L) {
        out->type = NO_OP;
    } else {
        out->type = RANGE;
    }
#undef L
#undef R
}
