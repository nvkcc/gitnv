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
        execlp("git", "git", "branch", "--show-current", NULL);
    } else {
        close(pd_b.fd[1]);
    }

    waitpid(pd_b.pid, NULL, 0);

    /// `git branch` byte buffer.
    char buf_b[GIT_BRANCH_BUF_MAX];
    /// `git remote` byte buffer.
    char buf_r[GIT_REMOTE_BUF_MAX];

    // By construction, buf_b_len < GIT_BRANCH_BUF_MAX.
    const int buf_b_len = read(pd_b.fd[0], buf_b, GIT_BRANCH_BUF_MAX - 1);
    buf_b[buf_b_len] = '\0';
    debug_printf("Bytes read from `git branch`: %d", buf_b_len);

    // `git branch --show-current` returned an empty string. This is not a git
    // repository. Do nothing and end early.
    if (buf_b_len == 0) {
        return 0;
    }

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
        debug_printf("No remote found. Exit code: %d", git_remote_exit_code);
        SEND_STDOUT("%F{241}(");
        buf_b[buf_b_len - 1] = ')';
        write(STDOUT_FILENO, buf_b, buf_b_len);
        return 0;
    }

    // By construction, buf_r_len < GIT_REMOTE_BUF_MAX.
    const int buf_r_len = read(pd_r.fd[0], buf_r, GIT_REMOTE_BUF_MAX - 1);
    debug_printf("Bytes read from `git remote`: %d", buf_r_len);
    buf_r[buf_r_len] = '\0';

    // Remote is empty. Just return the branch name.
    if (git_remote_exit_code != 0) {
        debug_printf("No remote found. Exit code: %d", git_remote_exit_code);
        SEND_STDOUT("%F{241}(");
        buf_b[buf_b_len - 1] = ')';
        write(STDOUT_FILENO, buf_b, buf_b_len);
        return 0;
    }

    char *remote_start = buf_r + buf_r_len - 1;
    while (*remote_start != '/' && remote_start > buf_r) {
        remote_start--;
    }
    buf_r[buf_r_len - 1] = ')';
    if (remote_start == buf_r) {
        // No path separator found in the URL. Just send the whole darn remote.
        SEND_STDOUT("%F{241}(%F{246}");
        write(STDOUT_FILENO, buf_b, buf_b_len - 1);
        SEND_STDOUT("%F{241}");
        write(STDOUT_FILENO, buf_r, buf_r_len);
    } else {
        debug_printf("GOT HERE", 0);
        SEND_STDOUT("%F{241}(%F{246}");
        write(STDOUT_FILENO, buf_b, buf_b_len - 1);
        SEND_STDOUT("%F{241}");
        write(STDOUT_FILENO, remote_start, buf_r_len - (remote_start - buf_r));
    }

    return 0;
}
