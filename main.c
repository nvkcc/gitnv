#include "cache.h"
#include "config.h"
#include "debug.h"
#include "log.h"
#include "state.h"
#include "util.h"
#include <cwalk.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int gather_aliases(git_config *config) {
    char GIT_ALIASES[GITNV_MAX_ALIASES][2][GITNV_ALIAS_MAX_LEN];
    int GIT_ALIASES_LEN[GITNV_MAX_ALIASES][2];
    int NUM_GIT_ALIASES;

    int i = 0, key_len, val_len, err;
    git_config_iterator *it;
    git_config_iterator_glob_new(&it, config, "^alias.");
    git_config_entry *entry;
    for (; (err = git_config_next(&entry, it)) != GIT_ITEROVER; i++) {
        // TODO: handle the `err`.
        if (i == GITNV_MAX_ALIASES) {
            SEND_STDERR("Exceeded maximum number of git aliases.");
            return 1;
        }
        key_len = strlen(entry->name);
        val_len = strlen(entry->value);
        if (key_len > GITNV_ALIAS_MAX_LEN || val_len > GITNV_ALIAS_MAX_LEN) {
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
    for (int i = 1, l;;) {
        status_ptr = status_buf + (l = COUNT_DIGITS(i) + 1);
        if (fgets(status_ptr, sizeof(status_buf) - l, status_f) == NULL) {
            break;
        }
        n = strlen(status_ptr);
        if (i > GITNV_MAX_CACHE_NUMBER) {
            write(STDOUT_FILENO, status_ptr, n);
            continue;
        }
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

    char cache_filepath[GITNV_MAX_PATH_LEN];
    gitnv_state_get_cache_filepath(z, cache_filepath, GITNV_MAX_PATH_LEN);

    /// Nothing to write to cache
    if (cache_ptr == cache_buf) {
        log_info("Nothing to update cache.");
        return 0;
    }

    /// Write to the cache file.
    FILE *cache_f = fopen(cache_filepath, "w");
    if (cache_f == NULL) {
        write_stderr("Failed to open cache file.");
        return 1;
    }
    if (fwrite(cache_buf, cache_ptr - cache_buf, 1, cache_f) < 1) {
        write_stderr("Failed to write to cache file.");
    }
    fclose(cache_f);
    return 0;
}

pid_t gitnv_non_status(int argc, char *argv[], GitnvState *z) {
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
            gitnv_cache_load(cache, gitnv_state_git_dir(z), cache_f);
            fclose(cache_f);
        }
        log_info("Cache initialized!");
        // unsigned int n = gitnv_cache_len(cache), i;
        // for (i = 0; i < n; ++i) {
        //     char *p = gitnv_cache_get_checked(cache, i);
        //     if (p != NULL) { printf("[%d] %s\n", i, p); }
        // }
    }

    int i, j, k;
    parsed_arg pa[argc];

    // TODO: locate the git command ("add"/"commit"/"reset"/etc.) index first,
    // and then parse the args behind that.
    for (i = 1; i < argc; ++i) {
        parse_arg(argv[i], &pa[i]);
    }

    // Total args to send to execvp. 1 for "git", 1 for NULL.
    int num_args = 2;

    for (i = 1; i < argc; ++i) {
        switch (pa[i].type) {
        case NO_OP:
        case SINGLE:
            num_args++;
            break;
        case RANGE:
            num_args += pa[i].val.range[1] - pa[i].val.range[0] + 1;
            break;
        }
    }

    char *args[num_args];
    args[0] = "git";

    for (i = 1, j = 1; i < argc; ++i) {
        switch (pa[i].type) {
        case NO_OP:
            args[j] = argv[i];
            log_trace("NAssigned %d/%d : %s", j, num_args, args[j]);
            j++;
            break;
        case SINGLE:
            args[j] = gitnv_cache_get_checked(cache, pa[i].val.single - 1);
            if (args[j] == NULL) {
                args[j] = argv[i];
            }
            log_trace("SAssigned %d/%d : %s", j, num_args, args[j]);
            j++;
            break;
        case RANGE:
            for (k = pa[i].val.range[0]; k <= pa[i].val.range[1]; ++k) {
                args[j] = gitnv_cache_get_checked(cache, k - 1);
                if (args[j] == NULL) {
                    continue;
                }
                log_trace("RAssigned %d/%d : %s", j, num_args, args[j]);
                j++;
            }
            break;
        }
    }
    args[j] = NULL;

#ifdef DEBUG_MODE
    for (int i = 0; i < num_args; ++i) {
        log_trace("[%d] = %s", i, args[i]);
    }
    log_info("Git non-status function exited!");
#endif

    // TODO: handle the failure case.
    execvp("git", args);
    return 1;
}

int main_inner(int argc, char *argv[], char *current_dir) {
    int err;
    log_trace("[git-nv] CURRENT_DIR = %s", current_dir);
    for (int i = 0; i < argc; ++i) {
        log_trace("arg[%d] = %s", i, argv[i]);
    }
    GitnvState *z;
    err = gitnv_state_new(&z, current_dir);
    if (err != 0) {
        argv[0] = "git";
        // TODO: handle when this fails.
        execvp("git", argv);
        return 32768;
    }

    // We only support enumerating the values of the vanilla `git nv status`
    // command for now. Nothing else.
    if (argc == 2 && strncmp(argv[1], "status", 6) == 0) {
        err = gitnv_status(z);
    } else {
        pid_t pid;
        if ((pid = fork()) == 0) {
            gitnv_non_status(argc, argv, z);
        } else {
            waitpid(pid, &err, 0);
            err >>= 8;
            log_trace("gitnv-non-status pid: %d, exit_code = %d", pid, err);
        }
    }
    gitnv_state_free(z);
    return err;
}

int main(int argc, char *argv[]) {
    log_set_level(LOG_TRACE);
    log_set_quiet(1);
    FILE *log_file = fopen("/home/khang/.local/state/git-nv.log", "w");
    log_add_fp(log_file, LOG_TRACE);
    char current_dir[GITNV_MAX_PATH_LEN];

    argv[0] = "git";
    if (!getcwd(current_dir, GITNV_MAX_PATH_LEN)) {
        write_stderr("Failed to get current working directory.");
        return 1;
    }
    log_trace("Inititalizing libgit2...");
    git_libgit2_init();
    log_trace("Inititalized libgit2.");
    int result = main_inner(argc, argv, current_dir);
    log_trace("Shutting down libgit2...");
    git_libgit2_shutdown();
    log_trace("libgit2 has been shut down.");
    log_trace("Exit code: %d", result);
    return result;
}
