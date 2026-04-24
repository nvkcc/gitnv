#include "util.h"
#include "config.h"
#include "log.h"
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
        log_trace("checking [%d] = %c", len, s[len]);
        if (!isdigit(s[len])) {
            log_trace("fail!");
            return 0;
        }
    }
    log_trace("pass! with %d", atoi(s));
    return atoi(s);
}

void parse_arg(char *arg, parsed_arg *out) {
    /// Length of `arg` without the NUL byte.
    int n = strlen(arg);
    log_trace("\x1b[31mstrlen\x1b[m = %d", n);
    char *dots;
    // Try to search for the ".." substring in the arg.
    if ((dots = memmem(arg, n, "..", 2)) == NULL) {
        // Either a regular pathspec, or a single number.
        n = antoi(arg, n);
        log_trace("single n = %d", n);
        if (n > 0) {
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
    log_trace("checkpt = %d", (arg + n - (dots + 2)));
    L = antoi(arg, dots - arg);
    R = antoi(dots + 2, arg + n - (dots + 2));
    if (R > GITNV_MAX_CACHE_NUMBER) {
        R = GITNV_MAX_CACHE_NUMBER;
    }
    if (L == 0 || R < L) {
        out->type = NO_OP;
    } else if (R == L) {
        out->type = SINGLE;
        out->val.single = R;
    } else {
        out->type = RANGE;
    }
#undef L
#undef R
}
