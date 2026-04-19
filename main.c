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
#define SEND_STDERR_LN(msg)                                                    \
    SEND_STDERR(msg);                                                          \
    write(STDERR_FILENO, "\n", 1);
#define SEND_STDOUT(msg) write(STDOUT_FILENO, msg, sizeof(msg));
#define SEND_STDOUT_LN(msg)                                                    \
    SEND_STDOUT(msg);                                                          \
    write(STDOUT_FILENO, "\n", 1);

struct pipedata {
    int fd[2];
    pid_t pid;
};

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

typedef struct {
    git_buf git_dir;
    git_repository *repo;
} GitnvState;

int gitnv_state_new(GitnvState **out) {
    GitnvState *z = malloc(sizeof(GitnvState));

    // Find the path to the git directory. This is the directory with
    // "branches/", "refs/", "HEAD", and so on.
    if (git_repository_discover(&z->git_dir, CURRENT_DIR, 0, NULL) != 0) {
        SEND_STDERR_LN("Not in a git repository.");
        free(z);
        return 1;
    }
    debug_printf("Found git dir: %s", z->git_dir.ptr);

    // Open the git repository with libgit2.
    if (git_repository_open(&z->repo, z->git_dir.ptr) != 0) {
        SEND_STDERR_LN("Failed to open git repository in libgit2.");
        git_buf_free(&z->git_dir);
        git_repository_free(z->repo);
        free(z);
        return 1;
    }

    *out = z;
    return 0;
}

int gitnv_state_free(GitnvState *state) {
    git_buf_free(&state->git_dir);
    git_repository_free(state->repo);
    free(state);
    return 0;
}

/// The custom opinionated `git-nv status` output.
int status(GitnvState *z) {
    debug_printf("Running `git nv status!`", 0);
    git_status_list *ls;
    git_status_options opts = GIT_STATUS_OPTIONS_INIT;
    opts.flags = GIT_STATUS_OPT_INCLUDE_UNTRACKED |
                 GIT_STATUS_OPT_RENAMES_HEAD_TO_INDEX |
                 GIT_STATUS_OPT_SORT_CASE_SENSITIVELY;
    git_status_list_new(&ls, z->repo, NULL);
    const int n = git_status_list_entrycount(ls);
    for (int i = 0; i < n; ++i) {
        const git_status_entry *entry = git_status_byindex(ls, i);
        debug_printf("%s", entry->index_to_workdir->new_file.path);
        debug_printf("%s", entry->index_to_workdir->old_file.path);
    }
    git_status_byindex(ls, 1);
    git_status_list_free(ls);
    return 0;
}

int non_status_git_command(int argc, char *argv[], GitnvState *z) { return 0; }

int main_inner(int argc, char *argv[]) {
    int err;
    debug_printf("START EXECUTION", 0);

    for (int i = 0; i < argc; ++i) {
        debug_printf("arg[%d] = %s", i, argv[i]);
    }
    debug_printf("CURRENT_DIR = %s", CURRENT_DIR);
    GitnvState *z;
    err = gitnv_state_new(&z);
    if (err != 0) {
        SEND_STDERR_LN("Failed to initialize git-nv state.");
        return err;
    }

    // if (argc == 2 && strncmp(argv[1], "status", 6) == 0) {
    //     status(z);
    // } else {
    //     non_status_git_command(argc, argv, z);
    //     git_config *config;
    //     git_repository_config(&config, z->repo);
    //     if ((err = gather_aliases(config)) != 0) {
    //         SEND_STDOUT("gather_aliases failed.");
    //     } else {
    //         debug_printf("Number of git aliases: %d", NUM_GIT_ALIASES);
    //     }
    //     git_config_free(config);
    // }
    gitnv_state_free(z);
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
