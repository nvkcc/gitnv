#include <git2.h>
#include <git2/repository.h>

#include <cwalk.h>

#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "kstr.h"

#define GITNV_MAX_PATH_LEN 1024
#define GIT_BRANCH_BUF_MAX 256
#define GIT_REMOTE_BUF_MAX (1024 - GIT_BRANCH_BUF_MAX)
#define GITNV_CACHE_FILENAME "gitnv.txt"

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

static char CURRENT_DIR[GITNV_MAX_PATH_LEN];

#define GIT_ALIAS_MAX_LEN 16
#define MAX_GIT_ALIASES 10
static char GIT_ALIASES[MAX_GIT_ALIASES][2][GIT_ALIAS_MAX_LEN];
static int GIT_ALIASES_LEN[MAX_GIT_ALIASES][2];
static int NUM_GIT_ALIASES;

int gather_aliases(git_config *config) {
    int i = 0, key_len, val_len, err;
    git_config_iterator *it;
    git_config_iterator_glob_new(&it, config, "^alias.");
    git_config_entry *entry;
    for (; (err = git_config_next(&entry, it)) != GIT_ITEROVER; i++) {
        // TODO: handle the `err`.
        if (i == MAX_GIT_ALIASES) {
            SEND_STDERR("Exceeded maximum number of git aliases.");
            return 1;
        }
        key_len = strlen(entry->name);
        val_len = strlen(entry->value);
        if (key_len > GIT_ALIAS_MAX_LEN || val_len > GIT_ALIAS_MAX_LEN) {
            SEND_STDERR("There is a git alias that is longer than the maximum "
                        "length allowed.");
            return 1;
        }
        strncpy(GIT_ALIASES[i][0], entry->name, key_len);
        strncpy(GIT_ALIASES[i][1], entry->value, val_len);
    }
    NUM_GIT_ALIASES = i;
    git_config_iterator_free(it);
    return 0;
}

void create_cache_file(git_buf *git_dir) {
    char cache_filepath[GITNV_MAX_PATH_LEN];
    cwk_path_join(git_dir->ptr, GITNV_CACHE_FILENAME, cache_filepath,
                  GITNV_MAX_PATH_LEN);
    FILE *f = fopen(cache_filepath, "w+");
    fwrite("HEYYYYYYYYYYYYYYYYYYYYYYYY", 10, 1, f);
    fclose(f);
    debug_printf("%s", cache_filepath);

    // The prefix to be pre-pended to every "git status" entry to make it such
    // that each one is relative to `git_dir`. Note that "git status" shows
    // filepaths relative to the current working directory (`CURRENT_DIR`).
    char prefix[GITNV_MAX_PATH_LEN];

    cwk_path_get_relative(git_dir->ptr, CURRENT_DIR, prefix,
                          GITNV_MAX_PATH_LEN);
    debug_printf("%s", prefix);
    // cwk_path_get_relative(b, a, buf, GITNV_MAX_PATH_LEN);
    // debug_printf("%s", buf);
}

int main_inner(int argc, char *argv[]) {
    int err;
    debug_printf("START EXECUTION", 0);
    debug_printf("CURRENT_DIR = %s", CURRENT_DIR);
    git_buf git_dir = {0};
    git_repository *repo;
    git_config *config;
    // cwk_path_join();

    err = git_repository_discover(&git_dir, CURRENT_DIR, 0, NULL);
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
    git_repository_config(&config, repo);
    if ((err = gather_aliases(config)) != 0) {
        SEND_STDOUT("gather_aliases failed.");
        return err;
    }
    debug_printf("Number of git aliases: %d", NUM_GIT_ALIASES);
    create_cache_file(&git_dir);

    return 0;
}

int main(int argc, char *argv[]) {
    argv[0] = "git";
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
