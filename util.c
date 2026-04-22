#include "util.h"
#include <stdbool.h>

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
