#include <stdint.h>

#define PIPE_OR_RETURN(fd)                                                     \
    if (pipe(fd) == -1) {                                                      \
        perror("pipe failed");                                                 \
        return 1;                                                              \
    }

#define FORK_OR_RETURN(pid)                                                    \
    if ((pid = fork()) == -1) {                                                \
        perror("fork failed");                                                 \
        return 1;                                                              \
    }

#ifdef __cplusplus
extern "C" {
#endif

// Simple function that removes ANSI color codes by removing everything between
// '\x1b' and 'm' (inclusive, of course). `len` should be the length of the
// string including the NUL byte. Returns the new length of the string (also
// including the NUL byte).
int uncolor(char *b, int len);

int parse_args(const char *arg, uint64_t *cache_mask);

enum arg_type {
    // Treat the arg like a regular pathspec.
    NO_OP,
    SINGLE,
    RANGE
};

union arg {
    int single;
    int range[2];
};

typedef struct parsed_arg {
    enum arg_type type;
    union arg val;
} parsed_arg;

void parse_arg(char *arg, parsed_arg *);

#ifdef __cplusplus
}
#endif
