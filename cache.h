#ifndef e4b4a5efbd4468576089c412040b632f63324d78
#define e4b4a5efbd4468576089c412040b632f63324d78 1

#include <stdio.h>

typedef struct GitnvCache GitnvCache;

// @return 0 or an error code
void gitnv_cache_new(GitnvCache **);
void gitnv_cache_free(GitnvCache *);

/// Gets a raw pointer to a cache entry. Returns NULL if index is greater than
/// GITNV_MAX_CACHE_NUMBER. Note that each entry is allocated GITNV_MAX_PATH_LEN
/// bytes.
char *gitnv_cache_get_raw(GitnvCache *, unsigned int);

/// Gets a pointer a cache entry. Returns NULL if out of range. Note that each
/// entry is allocated GITNV_MAX_PATH_LEN bytes.
char *gitnv_cache_get_checked(GitnvCache *, unsigned int);

/// Loads the cache from a cache file.
void gitnv_cache_load(GitnvCache *, char *git_dir, FILE *);

/// Number of items in the cache.
unsigned int gitnv_cache_len(GitnvCache *);

#endif
