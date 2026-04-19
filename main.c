#include "git2.h"

#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define GIT_BRANCH_BUF_MAX 256
#define GIT_REMOTE_BUF_MAX (1024 - GIT_BRANCH_BUF_MAX)

// Ban list.
// 1. printf: in production we want zero allocations.
#define BANNED(func) sorry__##func##__is_a_banned_function
#undef printf
#define printf(...) BANNED(strcpy)

#define STARTS_WITH(haystack, prefix)                                          \
    (strncmp(haystack, prefix, sizeof(prefix) - 1) == 0)

// #define DEBUG
#ifdef DEBUG
#include <stdio.h>
static int DEBUG_ID = 0;
#define debug_printf(format, ...)                                              \
    fprintf(stderr, "[\x1b[32mINFO\x1b[m] (%d) " format "\n", DEBUG_ID++,      \
            __VA_ARGS__)
#else
#define debug_printf(...)
#endif

#define SEND_STDERR(msg) write(STDERR_FILENO, msg, sizeof(msg));
#define SEND_STDERR_LN(msg) SEND_STDERR(msg), write(STDERR_FILENO, "\n", 1);
#define SEND_STDOUT(msg) write(STDOUT_FILENO, msg, sizeof(msg));
#define SEND_STDOUT_LN(msg) SEND_STDOUT(msg), write(STDOUT_FILENO, "\n", 1);

struct pipedata {
    int fd[2];
    pid_t pid;
};

#define PIPE_AND_FORK(pd, COMMAND)                                             \
    if (pipe(pd.fd) == -1) {                                                   \
        SEND_STDERR("pipe");                                                   \
        SEND_STDERR(" failed. This is necessary to run `git ");                \
        SEND_STDERR(#COMMAND);                                                 \
        SEND_STDERR("`.");                                                     \
        return 1;                                                              \
    } else if ((pd.pid = fork()) == -1) {                                      \
        SEND_STDERR("fork");                                                   \
        SEND_STDERR(" failed. This is necessary to run `git ");                \
        SEND_STDERR(#COMMAND);                                                 \
        SEND_STDERR("`.");                                                     \
        return 1;                                                              \
    }

static char CURRENT_DIR[1024];

int main(int argc, char *argv[]) {
    // git_libgit2_init();

    if (!getcwd(CURRENT_DIR, 1024)) {
        SEND_STDERR("Failed to get current working directory.");
        return 1;
    }
}

// debug_printf("\x1b[33mDEBUG MODE\x1b[m", 0);
//
// // Pipe data for `git branch`.
// struct pipedata pd_b;
// PIPE_AND_FORK(pd_b, branch);
//
// /* Child process: `git branch` */
// if (pd_b.pid == 0) {
//     // Capture `git branch` STDOUT.
//     dup2(pd_b.fd[1], STDOUT_FILENO);
//     // Don't send anything to STDERR.
//     close(STDERR_FILENO);
//     close(pd_b.fd[0]), close(pd_b.fd[1]);
//     return execlp("git", "git", "branch", "--show-current", NULL);
// } else {
//     close(pd_b.fd[1]);
// }
//
// waitpid(pd_b.pid, NULL, 0);
// for (int i = 0; i < argc; i++) {
//     debug_printf(" * [%d] = %s", i, argv[i]);
// }
// argv[0] = "git";
//
// return execvp("git", argv);
