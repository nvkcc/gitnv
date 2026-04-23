#ifndef _GITNV_STATE_H
#define _GITNV_STATE_H 1

#include <git2.h>

typedef struct GitnvState GitnvState;

// @return 0 or an error code
int gitnv_state_new(GitnvState **out, char *current_dir);
void gitnv_state_free(GitnvState *);

void gitnv_state_get_cache_filepath(GitnvState *, char *buffer, int len);

// The prefix to be pre-pended to every "git status" entry to make it such
// that each one is relative to `git_dir`. Note that "git status" shows
// filepaths relative to the current working directory (`CURRENT_DIR`).
void gitnv_state_get_prefix(GitnvState *, char *buffer, int len);

#endif
