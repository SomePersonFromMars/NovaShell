#include "executor.h"

#include "config.h"
#include "siparse.h"
#include "common.h"

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

typedef struct execargs {
    size_t argc;
    char** argv;
} execargs;

#define MAX_ARGS (MAX_LINE_LENGTH/2)
static char* argv_pool[MAX_ARGS];

// Returns pointers to statically allocated pools.
execargs
getexecargsfromcommand(command *pcmd)
{
    execargs r;
    r.argc = 0;
    r.argv = argv_pool;

	argseq * argseq = pcmd->args;
	do{
		r.argv[r.argc++] = argseq->arg;
		argseq= argseq->next;
	}while(argseq!=pcmd->args);
    r.argv[r.argc] = NULL;
    return r;
}

void executecommand(command *pcmd) {
    if (pcmd==NULL){
        // Do nothing.
        return;
    }

    execargs exec_args = getexecargsfromcommand(pcmd);
    pid_t child_pid = fork();
    if (child_pid == 0) {
        execvp(exec_args.argv[0], exec_args.argv);
        switch (errno) {
            case ENOENT:
                fprintf(stderr, "%s: no such file or directory\n", exec_args.argv[0]);
                break;
            case EACCES:
                fprintf(stderr, "%s: permission denied\n", exec_args.argv[0]);
                break;
            default:
                fprintf(stderr, "%s: exec error\n", exec_args.argv[0]);
                break;
        }
        exit(EXEC_FAILURE);
    } else if (child_pid < 0) {
        LOG_ERROR("fork() failed.");
    } else {
        waitpid(child_pid, NULL, 0);
    }

    // TODO handle redirections, background, etc.
}
