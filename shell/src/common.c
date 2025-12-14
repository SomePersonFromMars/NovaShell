#include "common.h"

#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

int
countopenfds(void)
{
    int fd_count = 0;
    for (int fd = 0; fd < FOPEN_MAX; fd++){
        if ((fcntl(fd, F_GETFD) != -1) || (errno != EBADF))
            fd_count++;
    }
    return fd_count;
}

int
issignalblocked(int signo)
{
    sigset_t current_mask;
    const int r = sigprocmask(SIG_BLOCK, NULL, &current_mask);
    assert(r == 0);
    return sigismember(&current_mask, signo);
}
