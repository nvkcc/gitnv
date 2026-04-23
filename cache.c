#include "cache.h"
#include "config.h"
#include <stdlib.h>
#include <string.h>

typedef struct GitnvCache {
    char entries[GITNV_MAX_CACHE_NUMBER][GITNV_MAX_PATH_LEN];
    /// Number of valid entries in cache.
    unsigned int len;
} GitnvCache;

void gitnv_cache_new(GitnvCache **out) {
    GitnvCache *cache = malloc(sizeof(GitnvCache));
    *out = cache;
}

void gitnv_cache_free(GitnvCache *cache) { free(cache); }

char *gitnv_cache_get_raw(GitnvCache *z, unsigned int index) {
    if (index > GITNV_MAX_CACHE_NUMBER) {
        return NULL;
    }
    return z->entries[index];
}

char *gitnv_cache_get_checked(GitnvCache *z, unsigned int index) {
    if (index >= z->len) {
        return NULL;
    }
    return z->entries[index];
}

void gitnv_cache_load(GitnvCache *z, FILE *cache_f) {
    char *ptr;
    int i;
    for (i = 0; i < GITNV_MAX_CACHE_NUMBER; ++i) {
        if (fgets(z->entries[i], GITNV_MAX_PATH_LEN, cache_f) == NULL) {
            break;
        }
        printf("[%d] %s", i, z->entries[i]);
        // Successful insertion.
        z->len = i + 1;
        if ((ptr = memchr(z->entries[i], '\n', GITNV_MAX_PATH_LEN))) {
            *ptr = '\0';
        }
    }
}

unsigned int gitnv_cache_len(GitnvCache *z) { return z->len; }
