#ifndef a2f7a3b1086d5d6b1cdec43762eee17380a99a43
#define a2f7a3b1086d5d6b1cdec43762eee17380a99a43 1

#include <stdio.h>
#include <unistd.h>

#define SEND_STDERR(msg) write(STDERR_FILENO, msg, sizeof(msg));
#define SEND_STDERR_LN(msg) write(STDERR_FILENO, msg "\n", sizeof(msg) + 1);

#define SEND_STDOUT(msg) write(STDOUT_FILENO, msg, sizeof(msg));
#define SEND_STDOUT_LN(msg) write(STDOUT_FILENO, msg "\n", sizeof(msg) + 1);

#endif
