#ifndef _GITNU_DEBUG_H
#define _GITNU_DEBUG_H 1

#include <stdio.h>
#include <unistd.h>

#ifdef DEBUG
#define debug_printf(format, ...)                                              \
    fprintf(stderr, "[\x1b[32mINFO\x1b[m] " format "\n", __VA_ARGS__)
#else
#define debug_printf(...)
#endif

#define SEND_STDERR(msg) write(STDERR_FILENO, msg, sizeof(msg));
#define SEND_STDERR_LN(msg) write(STDERR_FILENO, msg "\n", sizeof(msg) + 1);

#define SEND_STDOUT(msg) write(STDOUT_FILENO, msg, sizeof(msg));
#define SEND_STDOUT_LN(msg) write(STDOUT_FILENO, msg "\n", sizeof(msg) + 1);

#endif
