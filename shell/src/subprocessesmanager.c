#include "subprocessesmanager.h"

#include <bits/types/sigset_t.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <stddef.h>
#include <signal.h>
#include <assert.h>
#include <stdio.h>

typedef struct childstatus {
    pid_t pid;
    int signal_received;
    bool exited;
    int exit_status;
} childstatus;

typedef struct pendingchildstatuses {
    size_t next_unread_status_id;
    size_t next_free_status_slot_id;
    childstatus child_statuses[MAX_CHILD_STATUSES_CNT];
} pendingchildstatuses;

static pendingchildstatuses pending_bg_children_statuses;
// Should be set to a 0-ended array of pids
static pid_t *foreground_processes = NULL;
static int running_fg_children_cnt = 0;

static volatile int sigchld_block_counter = 0;
sigset_t
blocksigchld(void)
{
    sigset_t original_set;
    sigset_t temporary_set;
    sigemptyset(&temporary_set);

    if (sigchld_block_counter++ == 0)
        sigaddset(&temporary_set, SIGCHLD);

    sigprocmask(SIG_BLOCK, &temporary_set, &original_set);
    return original_set;
}

void
unblocksigchld(const sigset_t *original_set)
{
    if (--sigchld_block_counter == 0) {
        sigprocmask(SIG_SETMASK, original_set, NULL);
    }
}

void
setforegroundprocesses(pid_t new_foreground_processes_list[], int new_running_fg_children_count)
{
    const sigset_t original_set = blocksigchld();
    foreground_processes = new_foreground_processes_list;
    running_fg_children_cnt = new_running_fg_children_count;
    unblocksigchld(&original_set);
}

bool
isforegroundprocess(pid_t child_pid)
{
    const sigset_t original_set = blocksigchld();
    bool result = false;
    if (foreground_processes) {
        pid_t *next_process = foreground_processes;
        while (*next_process) {
            if (child_pid == *next_process) {
                result = true;
                break;
            }
            ++next_process;
        }
    }
    unblocksigchld(&original_set);
    return result;
}

void
addpendingbgchildstatus(childstatus status)
{
    const size_t new_head_id
        = (pending_bg_children_statuses.next_free_status_slot_id+1)%MAX_CHILD_STATUSES_CNT;
    if (new_head_id == pending_bg_children_statuses.next_unread_status_id) {
        // Ignore the oldest status
        pending_bg_children_statuses.next_unread_status_id
            = (pending_bg_children_statuses.next_unread_status_id+1)%MAX_CHILD_STATUSES_CNT;
    }
    pending_bg_children_statuses.child_statuses[pending_bg_children_statuses.next_free_status_slot_id]
        = status;
    pending_bg_children_statuses.next_free_status_slot_id = new_head_id;
}

childstatus*
getpendingbgchildstatus(void)
{
    const sigset_t original_set = blocksigchld();
    size_t * const head_id = &pending_bg_children_statuses.next_unread_status_id;
    size_t * const tail_id = &pending_bg_children_statuses.next_free_status_slot_id;
    if (*head_id == *tail_id) {
        unblocksigchld(&original_set);
        return NULL;
    }
    const size_t old_head_id = *head_id;
    const size_t new_head_id = (*head_id+1)%MAX_CHILD_STATUSES_CNT;
    *head_id = new_head_id;

    unblocksigchld(&original_set);
    return &pending_bg_children_statuses.child_statuses[old_head_id];
}

void
printpendingbgchildrenstatuses(void)
{
    const sigset_t original_set = blocksigchld();
    childstatus *status;
    while ((status = getpendingbgchildstatus())) {
        if (status->exited) {
            fprintf(stdout,
                "Background process %d terminated. (exited with status %d)\n",
                status->pid, status->exit_status);
        } else {
            fprintf(stdout,
                "Background process %d terminated. (killed by signal %d)\n",
                status->pid, status->signal_received);
        }
    }
    unblocksigchld(&original_set);
}

void
chldsigaction(int sig, siginfo_t *info, void *ucontext)
{
    assert(sig == SIGCHLD);
    ++sigchld_block_counter;

    const pid_t child_pid = info->si_pid;
    const int signal_child_received = info->si_code;
    const bool child_exited = (info->si_code == CLD_EXITED);
    const int child_exit_status = info->si_status;

    pid_t r = waitpid(child_pid, NULL, 0);
    assert(r == child_pid);

    childstatus new_child_status;
    new_child_status.pid = child_pid;
    new_child_status.signal_received = signal_child_received;
    new_child_status.exited = child_exited;
    new_child_status.exit_status = child_exit_status;

    const bool fg_child = isforegroundprocess(child_pid);
    if (fg_child) {
        // TODO FIXME Assuming that the child has exited/terminated/etc. in some way.
        --running_fg_children_cnt;
    } else {
        addpendingbgchildstatus(new_child_status);
    }

    --sigchld_block_counter;
}

void
setupsubprocesseswatcherandsignals(void)
{
    #ifndef DEBUG
    struct sigaction int_act;
    int_act.sa_handler = SIG_IGN;
    sigaction(SIGINT, &int_act, NULL);
    #endif

    pending_bg_children_statuses.next_unread_status_id = 0;
    pending_bg_children_statuses.next_free_status_slot_id = 0;
    struct sigaction chld_act;
    chld_act.sa_sigaction = chldsigaction;
    chld_act.sa_flags = SA_SIGINFO | SA_RESTART;
    sigaction(SIGCHLD, &chld_act, NULL);
}

void
revertdefaultsignalhandlers(void)
{
    struct sigaction act;
    act.sa_handler = SIG_DFL;
    sigaction(SIGINT, &act, NULL);
}

void
waitforforegroundprocessestofinish(void)
{
    sigset_t original_set = blocksigchld();
    while (running_fg_children_cnt > 0) {
        sigsuspend(&original_set);
    }
    unblocksigchld(&original_set);
}
