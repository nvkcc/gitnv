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

void parse_arg(char *arg, parsed_arg *out) {
    /// Length of `arg` without the NUL byte.
    int n = strlen(arg);
    char *dots;
    // Try to search for the ".." substring in the arg.
    if ((dots = memmem(arg, n, "..", 2)) == NULL) {
        // Either a regular pathspec, or a single number.
        n = atoi(arg);
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
    *dots = '\0';
#define L out->val.range[0]
#define R out->val.range[1]
    L = atoi(arg);
    R = atoi(dots + 2);
    *dots = '.';
    if (L == 0 || !GITNV_IS_VALID_USER_INPUT_NUMBER(R) || R < L) {
        out->type = NO_OP;
    } else {
        out->type = RANGE;
    }
#undef L
#undef R
}
