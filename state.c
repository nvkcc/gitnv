#include "state.h"
#include "debug.h"

int gitnv_state_new(GitnvState **out, char *current_dir) {
    GitnvState *z = malloc(sizeof(GitnvState));
    z->current_dir = current_dir;

    // Find the path to the git directory. This is the directory with
    // "branches/", "refs/", "HEAD", and so on.
    if (git_repository_discover(&z->git_dir, z->current_dir, 0, NULL) != 0) {
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

void gitnv_state_free(GitnvState *state) {
    git_buf_free(&state->git_dir);
    git_repository_free(state->repo);
    free(state);
}
