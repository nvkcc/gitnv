#include "bufread.h"
#include "cache.h"
#include "config.h"
#include "debug.h"
#include "log.h"
#include "state.h"
#include "util.h"
#include <cwalk.h>
#include <git2.h>
#include <git2/repository.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

// Ban list.
// 1. printf: in production we want zero allocations.
#define BANNED(func) sorry__##func##__is_a_banned_function
// #undef printf
// #define printf(...) BANNED(strcpy)

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
    log_info("%s", cache_filepath);

    // The prefix to be pre-pended to every "git status" entry to make it such
    // that each one is relative to `git_dir`. Note that "git status" shows
    // filepaths relative to the current working directory (`CURRENT_DIR`).
    char prefix[GITNV_MAX_PATH_LEN];

    cwk_path_get_relative(git_dir->ptr, CURRENT_DIR, prefix,
                          GITNV_MAX_PATH_LEN);
    log_info("%s", prefix);
    // cwk_path_get_relative(b, a, buf, GITNV_MAX_PATH_LEN);
    // log_info("%s", buf);
}

int gitnv_status_update_cache(GitnvState *z, git_status_list *gsl) {
    int n = git_status_list_entrycount(gsl), i = 0;
    for (const git_status_entry *entry; i < n; ++i) {
        entry = git_status_byindex(gsl, i);
        log_info("%s", entry->index_to_workdir->new_file.path);
        log_info("%s", entry->index_to_workdir->old_file.path);
    }
    return 0;
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
        execlp("git", "git", "-c", "color.status=always", "status", NULL);
    } else {
        close(fd[1]);
    }

    char prefix[GITNV_MAX_PATH_LEN];
    gitnv_state_get_prefix(z, prefix, GITNV_MAX_PATH_LEN);

    // 24kB cache buffer. To write to the file in one-shot later.
    char cache_buf[24 * 1024];
    char *cache_ptr = cache_buf;

    // The length of the current line of git status.
    int n;
    bool seen_untracked = 0;

    FILE *status_f = fdopen(fd[0], "rb");
    char status_buf[1024], *status_ptr;
    for (int i = 1, l; i <= GITNV_MAX_CACHE_NUMBER;) {
        status_ptr = status_buf + (l = COUNT_DIGITS(i) + 1);
        if (fgets(status_ptr, 1024 - l, status_f) == NULL) {
            break;
        }
        n = strlen(status_ptr);
        seen_untracked |= STARTS_WITH(status_ptr, "Untracked files:");
        // We only enumerate those lines that start with a '\t' character. Yes,
        // amazingly this identifier for lines of interest works.
        if (*status_ptr != '\t') {
            write(STDOUT_FILENO, status_ptr, n);
            continue;
        }
        snprintf(status_buf, l, "%d", i);
        write(STDOUT_FILENO, status_buf, n + l);
        n = uncolor(status_ptr, n);
        // Now, on to parsing the line.
        // https://libgit2.org/docs/reference/main/status/git_status_t.html

        // Example:
        // ```
        // Changes not staged for commit:
        // 1       modified:   core/status.rs
        //
        // Untracked files:
        // 2       core/line.rs
        // ```

        /// "modified", "deleted", "renamed", ...
        char *change_type = NULL;
        /// +1 char to skip the '\t' character.
        char *pathspec;
        char *pathspec_end = &status_ptr[n - 1];

        if (!seen_untracked) {
            change_type = status_ptr + 1; // +1 to skip the '\t' char.
            pathspec = memchr(change_type, ':', 16);
            *(pathspec++) = '\0';
            pathspec += strspn(pathspec, " \r\t");
        } else {
            pathspec = status_ptr + 1; // +1 to skip the '\t' char.
        }

        // Example:
        // ```
        // Changes to be committed:
        // 1       renamed:    README.md -> BUILD.md
        // ```
        if (change_type && STARTS_WITH(change_type, "renamed")) {
            pathspec_end = memmem(pathspec, pathspec_end - pathspec, "->", 2);
            while (*--pathspec_end == ' ') {
            }
            *++pathspec_end = '\n';
        }
        strncpy(cache_ptr, pathspec, (n = pathspec_end - pathspec + 1));
        *pathspec_end = '\0';
        int remaining_space = sizeof(cache_buf) - (cache_ptr - cache_buf);
        if (remaining_space < 0) {
            perror("Cache buffer ran out of space. Recompile with more stack "
                   "memory allocated.");
            return 1;
        }
        n = gitnv_state_resolve_pathspec(z, pathspec, cache_ptr,
                                         remaining_space);
        cache_ptr[n++] = '\n';
        cache_ptr += n;
        i++;
    }

    // write(STDOUT_FILENO, cache_buf, cache_ptr - cache_buf);

    char cache_filepath[GITNV_MAX_PATH_LEN];
    gitnv_state_get_cache_filepath(z, cache_filepath, GITNV_MAX_PATH_LEN);

    /// Write to the cache file.
    /// TODO: handle the case when `fopen()` fails.
    FILE *cache_f = fopen(cache_filepath, "w");
    fwrite(cache_buf, cache_ptr - cache_buf, 1, cache_f);
    fclose(cache_f);

    return 0;
}

/// Gets the additional number of argument this this argument expands out to.
/// "3"    -> 1
/// "8"    -> 1
/// "2..7" -> 6
/// "3..6" -> 4
/// "0..3" -> 1 (because 0 is an invalid index, so we treat that as a pathspec)
/// "0"    -> 1
/// "6..6" -> 1
/// "6..5" -> 1 (valid range but empty. So we'll treat that as a pathspec)
///
/// Since arg is guaranteed to come from `argv`, it is also guaranteed to be NUL
/// terminated.
int num_args(const char *arg, int64_t *cache_mask) {
    /// Length of `arg` without the NUL byte.
    int n = strlen(arg);
    char *dots;
    // Try to search for the ".." substring in the arg.
    if ((dots = memmem(arg, n, "..", 2)) == NULL) {
        // Either a regular pathspec, or a single number.
        n = atoi(arg);
        if (GITNV_IS_VALID_USER_INPUT_NUMBER(n)) {
            *cache_mask |= 1 << (n - 1);
        }
        return 1;
    }
    // Convert both the substrings on the left and right of the ".." to
    // integers. For that we need a NUL byte to be strategically placed.
    *dots = '\0';
    const int left = atoi(arg), right = atoi(dots + 2);
    *dots = '.';
    if (left == 0 || !GITNV_IS_VALID_USER_INPUT_NUMBER(right) || right < left) {
        // Treat the arg like a regular pathspec.
        return 1;
    }
    for (n = left; n <= right; ++n) {
        *cache_mask |= (1 << n);
    }
    return right - left + 1;
}

int gitnv_non_status(int argc, char *argv[], GitnvState *z) {
    GitnvCache *cache;
    gitnv_cache_new(&cache);
    {
        // Get the path to the cache file.
        char cache_filepath[GITNV_MAX_PATH_LEN];
        gitnv_state_get_cache_filepath(z, cache_filepath, GITNV_MAX_PATH_LEN);
        log_info("Initializing cache...");
        FILE *cache_f = fopen(cache_filepath, "r");
        /// TODO: handle (properly) the case when `fopen()` fails.
        if (cache_f) {
            gitnv_cache_load(cache, cache_f);
            fclose(cache_f);
        }
        log_info("Cache initialized!");
        // unsigned int n = gitnv_cache_len(cache), i;
        // for (i = 0; i < n; ++i) {
        //     char *p = gitnv_cache_get_checked(cache, i);
        //     if (p != NULL) { printf("[%d] %s\n", i, p); }
        // }
    }

    int total_args = argc;
    total_args += 1; // for the NULL at the end.

    for (int i = 0; i < argc; ++i) {
        isdigit(argv[i]);
        printf("[%d] %s\n", i, argv[i]);
    }

    gitnv_cache_free(cache);

    return 0;
}

int main_inner(int argc, char *argv[]) {
    int err;
    log_info("START EXECUTION", 0);

    for (int i = 0; i < argc; ++i) {
        log_info("arg[%d] = %s", i, argv[i]);
    }
    log_info("CURRENT_DIR = %s", CURRENT_DIR);
    GitnvState *z;
    err = gitnv_state_new(&z, CURRENT_DIR);
    if (err != 0) {
        SEND_STDERR_LN("Failed to initialize git-nv state.");
        return err;
    }

    // We only support enumerating the values of the vanilla `git nv status`
    // command for now. Nothing else.
    if (argc == 2 && strncmp(argv[1], "status", 6) == 0) {
        err = gitnv_status(z);
    } else {
        err = gitnv_non_status(argc, argv, z);
    }
    gitnv_state_free(z);
    return err;
}

int main(int argc, char *argv[]) {
    log_set_level(LOG_TRACE);
    // int n = 128;
    // char buf[n];
    // gitnv_buf_reader br = {.buf = buf};
    // gitnv_buf_reader_new(&br, STDIN_FILENO, n);
    // int i = 0;
    // while (gitnv_buf_reader_next(&br) == 0) {
    //     log_info("[%d] \x1b[33m|\x1b[m%s\x1b[33m|\x1b[m", ++i, buf);
    // }
    //
    // log_trace("END");
    //
    // return 0;

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
