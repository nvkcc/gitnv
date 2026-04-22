#include "bufread.h"
#include "config.h"
#include "debug.h"
#include "state.h"
#include <cwalk.h>
#include <git2.h>
#include <git2/repository.h>
#include <nk_log.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

// Ban list.
// 1. printf: in production we want zero allocations.
#define BANNED(func) sorry__##func##__is_a_banned_function
#undef printf
#define printf(...) BANNED(strcpy)

#define STARTS_WITH(haystack, prefix)                                          \
    (strncmp(haystack, prefix, sizeof(prefix) - 1) == 0)

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
    nklog_info("%s", cache_filepath);

    // The prefix to be pre-pended to every "git status" entry to make it such
    // that each one is relative to `git_dir`. Note that "git status" shows
    // filepaths relative to the current working directory (`CURRENT_DIR`).
    char prefix[GITNV_MAX_PATH_LEN];

    cwk_path_get_relative(git_dir->ptr, CURRENT_DIR, prefix,
                          GITNV_MAX_PATH_LEN);
    nklog_info("%s", prefix);
    // cwk_path_get_relative(b, a, buf, GITNV_MAX_PATH_LEN);
    // nklog_info("%s", buf);
}

int gitnv_status_update_cache(GitnvState *z, git_status_list *gsl) {
    int n = git_status_list_entrycount(gsl), i = 0;
    for (const git_status_entry *entry; i < n; ++i) {
        entry = git_status_byindex(gsl, i);
        nklog_info("%s", entry->index_to_workdir->new_file.path);
        nklog_info("%s", entry->index_to_workdir->old_file.path);
    }
}

/// The custom opinionated `git-nv status` output.
int gitnv_status(GitnvState *z) {
    int fd[2];
    pid_t pid;
    PIPE_OR_RETURN(fd);
    FORK_OR_RETURN(pid);
    if (pid == 0) {
        // Capture `git status` STDOUT.
        dup2(fd[1], STDOUT_FILENO);
        // Let stderr bypass by doing nothing to STDERR_FILENO.
        close(fd[0]);
        close(fd[1]);
        execlp("git", "git", "status", NULL);
    } else {
        close(fd[1]);
    }
    // // read(fd[0]);
    //
    // nklog_info("Running `git nv status!`", 0);
    // git_status_list *gsl;
    // git_status_options opts = GIT_STATUS_OPTIONS_INIT;
    // opts.flags = GIT_STATUS_OPT_INCLUDE_UNTRACKED |
    //              GIT_STATUS_OPT_RENAMES_HEAD_TO_INDEX |
    //              GIT_STATUS_OPT_SORT_CASE_SENSITIVELY;
    // git_status_list_new(&gsl, z->repo, &opts);
    // gitnv_status_update_cache(z, gsl);
    // git_status_list_free(gsl);
    return 0;
}

int gitnv_non_status(int argc, char *argv[], GitnvState *z) { return 0; }

int main_inner(int argc, char *argv[]) {
    int err;
    nklog_info("START EXECUTION", 0);

    for (int i = 0; i < argc; ++i) {
        nklog_info("arg[%d] = %s", i, argv[i]);
    }
    nklog_info("CURRENT_DIR = %s", CURRENT_DIR);
    GitnvState *z;
    err = gitnv_state_new(&z, CURRENT_DIR);
    if (err != 0) {
        SEND_STDERR_LN("Failed to initialize git-nv state.");
        return err;
    }

    // We only support enumerating the values of the vanilla `git nv status`
    // command for now. Nothing else.
    if (argc == 2 && strncmp(argv[1], "status", 6) == 0) {
        return gitnv_status(z);
    } else {
        gitnv_non_status(argc, argv, z);
    }
    //     non_status_git_command(argc, argv, z);
    //     git_config *config;
    //     git_repository_config(&config, z->repo);
    //     if ((err = gather_aliases(config)) != 0) {
    //         SEND_STDOUT("gather_aliases failed.");
    //     } else {
    //         nklog_info("Number of git aliases: %d", NUM_GIT_ALIASES);
    //     }
    //     git_config_free(config);
    // }
    gitnv_state_free(z);
    return 0;
}

int main(int argc, char *argv[]) {
    int n = 128;
    char buf[n];
    gitnv_buf_reader br = {.buf = buf};
    gitnv_buf_reader_new(&br, STDIN_FILENO, n);
    int i = 0;
    while (gitnv_buf_reader_next(&br) == 0) {
        nklog_info("[%d] \x1b[33m|\x1b[m%s\x1b[33m|\x1b[m", ++i, buf);
    }

    nklog_trace("END");

    return 0;
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
