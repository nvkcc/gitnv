#ifndef _GITNV_CONFIG_H
#define _GITNV_CONFIG_H 1

#define GITNV_MAX_PATH_LEN 1024
#define GITNV_CACHE_FILENAME "gitnv.txt"

/// Ensure that COUNT_DIGITS is still valid after changing this.
#define GITNV_MAX_CACHE_NUMBER 20

/// Operates on the assumption that X <= GITNV_MAX_CACHE_NUMBER.
#define COUNT_DIGITS(X) (X <= 0 ? -1 : (X < 10 ? 1 : 2))

#endif
