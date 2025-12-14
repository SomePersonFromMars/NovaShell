#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
#include "common.h"
#include <errno.h>

volatile int running_fg_children_cnt = 0;
void
runmultiplechildren(int children_cnt)
{
    running_fg_children_cnt = children_cnt;
    for (int i = 0; i < children_cnt; ++i) {
        pid_t child_pid = fork();
        if (child_pid == 0) {
            printf("%d\n", getpid());
            fflush(stdout);
            exit(0);
        } else {
        }
    }
}

void
sigchldaction(int sig, siginfo_t *info, void *ucontext)
{
    LOG_EXPR_INT(issignalblocked(SIGCHLD));
    assert(sig == SIGCHLD);
    int saved_errno = errno;

    pid_t child_pid;
    while ((child_pid = waitpid(-1, NULL, WNOHANG)) > 0) {
        LOG_EXPR_INT(child_pid);
        --running_fg_children_cnt;
    }
    fflush(stderr);

    errno = saved_errno;
}

sigset_t
blocksigchld(void)
{
    sigset_t original_set;
    sigset_t temporary_set;
    sigemptyset(&temporary_set);

    sigaddset(&temporary_set, SIGCHLD);
    LOG_INFO("Blocking SIGCHLD.");

    sigprocmask(SIG_BLOCK, &temporary_set, &original_set);
    return original_set;
}

void
unblocksigchld(const sigset_t *original_set)
{
    LOG_INFO("Unblocking SIGCHLD.");
    assert(sigismember(original_set, SIGCHLD) == 0);
    sigprocmask(SIG_SETMASK, original_set, NULL);
}

void
testmultiwait(void)
{
    struct sigaction chld_act;
    chld_act.sa_sigaction = sigchldaction;
    chld_act.sa_flags = SA_SIGINFO | SA_RESTART;
    sigaction(SIGCHLD, &chld_act, NULL);

    for (int i = 0; i < 1; ++i) {
        runmultiplechildren(500);

        sigset_t suspend_set = blocksigchld();
        while (running_fg_children_cnt > 0) {
            LOG_INFO("Before iteration. Children left = %d", running_fg_children_cnt);
            LOG_EXPR_INT(issignalblocked(SIGCHLD));
            LOG_EXPR_INT(sigismember(&suspend_set, SIGCHLD));
            LOG_INFO("Running suspend.");
            fflush(stderr);
            sigsuspend(&suspend_set);
            LOG_INFO("After suspend.");
            LOG_EXPR_INT(issignalblocked(SIGCHLD));
            fprintf(stderr, "\n\n");
            fflush(stderr);
        }
        unblocksigchld(&suspend_set);
    }
    LOG_EXPR_INT(running_fg_children_cnt);
}

int
main(int argc, char *argv[])
{
    testmultiwait();
}
