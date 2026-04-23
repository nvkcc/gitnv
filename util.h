#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Simple function that removes ANSI color codes by removing everything between
// '\x1b' and 'm' (inclusive, of course). `len` should be the length of the
// string including the NUL byte. Returns the new length of the string (also
// including the NUL byte).
int uncolor(char *b, int len);
int parse_args(const char *arg, uint64_t *cache_mask);

#ifdef __cplusplus
}
#endif
