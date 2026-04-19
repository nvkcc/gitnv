#include "git2.h"
#include "git2/config.h"
#include "git2/sys/config.h"
#include "git2/sys/repository.h"
#include "git2/types.h"

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
#define debug_printf(format, ...)                                              \
    fprintf(stderr, "[\x1b[32mINFO\x1b[m] " format "\n", __VA_ARGS__)
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

const char *TEST_PATH = "/home/khang/repos/financial-plan/c++";

int main_inner(int argc, char *argv[]) {
    int err;
    debug_printf("START EXECUTION", 0);
    git_buf git_dir = {0};
    git_repository *repo;

    err = git_repository_discover(&git_dir, TEST_PATH, 0, NULL);
    if (err != 0) {
        SEND_STDOUT("Not in a git repository.");
        return 1;
    }
    debug_printf("Found git dir: %s", git_dir.ptr);

    err = git_repository_open(&repo, git_dir.ptr);
    if (err != 0) {
        SEND_STDOUT("libgit2 failed to open the repository.");
        return 1;
    }
    // repo->_config;
    // repo->_config;

    debug_printf("GOT TO HERE!", 0);

    return 0;

    git_config *cfg;

    git_config_iterator *it;
    git_config_entry *entry;

    git_config_iterator_glob_new(&it, cfg, "^alias.");
    while (git_config_next(&entry, it) == 0) {
        debug_printf("%s", entry->name);
    }
    git_config_iterator_free(it);

    // for (; git_config_next())
    // *cfg, const char *regexp);

    return 0;
}

int main(int argc, char *argv[]) {
    if (!getcwd(CURRENT_DIR, 1024)) {
        SEND_STDERR("Failed to get current working directory.");
        return 1;
    }
    git_libgit2_init();
    int result = main_inner(argc, argv);
    git_libgit2_shutdown();
    return result;
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
