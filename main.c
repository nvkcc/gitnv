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

// Returns 64 if this function's stdout output is meant to be taken as the
// target directory for the `cd` command.
int main() {
    debug_printf("\x1b[33mDEBUG MODE\x1b[m", 0);

    // Pipe data for `git branch`.
    struct pipedata pd_b;
    PIPE_AND_FORK(pd_b, branch);

    /* Child process: `git branch` */
    if (pd_b.pid == 0) {
        // Capture `git branch` STDOUT.
        dup2(pd_b.fd[1], STDOUT_FILENO);
        // Don't send anything to STDERR.
        close(STDERR_FILENO);
        close(pd_b.fd[0]), close(pd_b.fd[1]);
        return execlp("git", "git", "branch", "--show-current", NULL);
    } else {
        close(pd_b.fd[1]);
    }

    waitpid(pd_b.pid, NULL, 0);

#define PREFIX "%F{241}("

    /// `git branch` byte buffer.
    char buf_b[GIT_BRANCH_BUF_MAX] = PREFIX;
    /// `git remote` byte buffer.
    char buf_r[GIT_REMOTE_BUF_MAX];
    char *ptr_b = buf_b + sizeof(PREFIX) - 1;

    debug_printf("pid = %d", pd_b.pid);

    // By construction, buf_b_len < GIT_BRANCH_BUF_MAX.
    const int bytes_read_b =
        read(pd_b.fd[0], ptr_b, GIT_BRANCH_BUF_MAX - sizeof(PREFIX));
    debug_printf("Bytes read from `git branch` = %d", bytes_read_b);

    // `git branch --show-current` returned an empty string. This is not a git
    // repository. Do nothing and end early.
    if (bytes_read_b == 0) {
        return 0;
    }
    const int buf_b_len = sizeof(PREFIX) + bytes_read_b - 1;
    debug_printf("Buffer length for `git branch` = %d", buf_b_len);

    // Carefully terminate the string.
    buf_b[buf_b_len] = '\0';

    // Set the last char (which is expected to be '\n') to the closing
    // parenthesis ')' because that's exactly what we'll print after the branch
    // in all scenarios.
    buf_b[buf_b_len - 1] = ')';

    // Pipe data for `git remote`.
    struct pipedata pd_r;
    PIPE_AND_FORK(pd_r, remote);

    /* Child process: `git remote` */
    if (pd_r.pid == 0) { //
        // Capture `git remote` STDOUT.
        dup2(pd_r.fd[1], STDOUT_FILENO);
        // Don't send anything to STDERR.
        close(STDERR_FILENO);
        close(pd_r.fd[0]), close(pd_r.fd[1]);
        execlp("git", "git", "config", "get", "remote.origin.url", NULL);
    } else {
        close(pd_r.fd[1]);
    }

    int git_remote_exit_code;
    waitpid(pd_r.pid, &git_remote_exit_code, 0);

    // There is no remote. Just return the branch name.
    if (git_remote_exit_code != 0) {
        debug_printf(
            "git config get remote.origin.url returned non-zero exit code: %d",
            git_remote_exit_code);
        write(STDOUT_FILENO, buf_b, buf_b_len);
        return 0;
    }

    // By construction, buf_r_len < GIT_REMOTE_BUF_MAX.
    const int buf_r_len = read(pd_r.fd[0], buf_r, GIT_REMOTE_BUF_MAX - 1);
    debug_printf("Bytes read from `git remote` = %d", buf_r_len);

    // Remote is empty. Just return the branch name.
    if (buf_r_len == 0) {
        debug_printf("git config get remote.origin.url returned empty string",
                     0);
        write(STDOUT_FILENO, buf_b, buf_b_len);
        return 0;
    }

    // Carefully terminate the string.
    buf_r[buf_r_len] = '\0';

    // Print the prefix first.
    SEND_STDOUT(PREFIX "%F{246}");

    // Search from the back for the '/' character.
    char *remote_start = buf_r + buf_r_len - 1;
    while (*remote_start != '/' && remote_start > buf_r) {
        remote_start--;
    }
    remote_start += *remote_start == '/';

    // If the '/' character is not found, print a ??? in the stead of the remote
    // name. This is because we at least know it exists, but we don't know as
    // what.
    if (remote_start == buf_r) {
        SEND_STDOUT("???%F{241}/");
        write(STDOUT_FILENO, buf_b, buf_b_len);
        return 0;
    }

    // Search from the `remote_start` for the '.' character.
    char *remote_end = remote_start;
    while (*remote_end != '.' && remote_end < buf_r + buf_r_len) {
        remote_end++;
    }
    *remote_end = '/';
    debug_printf("Made it to the end", 0);

    write(STDOUT_FILENO, remote_start, remote_end - remote_start + 1);
    SEND_STDOUT("%F{241}");
    write(STDOUT_FILENO, buf_b, buf_b_len);

    return 0;
}
