#ifndef a66e088220312305d74e0a8eacc6236f20b16425
#define a66e088220312305d74e0a8eacc6236f20b16425 1

#define GITNV_MAX_PATH_LEN 1024
#define GITNV_CACHE_FILENAME "gitnv.txt"
#define GITNV_ALIAS_MAX_LEN 16
#define GITNV_MAX_ALIASES 10

/// Feel free to change this to any value between 1 and 64 inclusive.
#define GITNV_MAX_CACHE_NUMBER 20

/// Operates on the assumption that X <= GITNV_MAX_CACHE_NUMBER.
#define COUNT_DIGITS(X) (X <= 0 ? -1 : (X < 10 ? 1 : 2))

#define GITNV_IS_VALID_USER_INPUT_NUMBER(N)                                    \
    (N != 0 && N <= GITNV_MAX_CACHE_NUMBER)

#endif
