#ifndef _GITNV_STATE_H
#define _GITNV_STATE_H 1

#include <git2.h>

typedef struct {
    git_buf git_dir;
    git_repository *repo;
    char *current_dir;
} GitnvState;

// @return 0 or an error code
int gitnv_state_new(GitnvState **out, char *current_dir);
void gitnv_state_free(GitnvState *);

void gitnv_state_get_cache_filepath(GitnvState *, char *buf, int len);

#endif
