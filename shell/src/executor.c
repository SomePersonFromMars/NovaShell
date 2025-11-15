#include "executor.h"

#include "builtins.h"
#include "config.h"
#include "siparse.h"
#include "common.h"

#include <errno.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <stdbool.h>

typedef struct execargs {
    size_t argc;
    char** argv;
} execargs;

typedef struct inoutdescriptors {
    int infd;
    int outfd;
    bool good;
} inoutdescriptors;

#define DEFAULT_FILE_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH)
#define MAX_ARGS ((MAX_LINE_LENGTH+1)/2)
static char* argv_pool[MAX_ARGS+1]; // The last one for terminating NULL

// Returns pointers to statically allocated pools.
execargs
getexecargsfromcommand(command *pcmd)
{
    execargs r;
    r.argc = 0;
    r.argv = argv_pool;

    argseq * argseq = pcmd->args;
    do {
        r.argv[r.argc++] = argseq->arg;
        argseq= argseq->next;
    } while(argseq!=pcmd->args);
    r.argv[r.argc] = NULL;
    return r;
}

static bool
handleopenerrno(const char * const filename)
{
    switch (errno) {
        case 0:
            return true;
            break;
        case EACCES:
            fprintf(stderr, "%s: permission denied\n", filename);
            return false;
            break;
        case ENOENT:
            fprintf(stderr, "%s: no such file or directory\n", filename);
            return false;
            break;
        default:
            LOG_ERROR("Unknown open() error.");
            return false;
            break;
    }
}

inoutdescriptors
getinoutdescriptorsfromcommand(command *pcmd)
{
    const char * input_filename  = NULL;
    const char * output_filename = NULL;
    const char * append_filename = NULL;

    redirseq * redirs = pcmd->redirs;
    if (redirs) {
        do {
            const int flags = redirs->r->flags;
            const char * const filename = redirs->r->filename;

            if (IS_RIN(flags)) {
                input_filename = filename;
            } else if (IS_ROUT(flags)) {
                output_filename = filename;
                append_filename = NULL;
            } else if (IS_RAPPEND(flags)) {
                output_filename = NULL;
                append_filename = filename;
            } else
                assert(("Unknown redirection type.", 0));

            redirs = redirs->next;
        } while (redirs!=pcmd->redirs);
    }

    inoutdescriptors descr;
    descr.infd = STDIN_FILENO;
    descr.outfd = STDOUT_FILENO;
    descr.good = true;

    if (input_filename != NULL) {
        errno = 0;
        const int fd = open(input_filename, O_RDONLY);
        if (fd > 0) descr.infd = fd;
        else descr.good = handleopenerrno(input_filename);
    }
    if (!descr.good) return descr;

    assert(output_filename == NULL || append_filename == NULL);
    if (output_filename != NULL) {
        errno = 0;
        const int fd = open(output_filename, O_CREAT | O_WRONLY | O_TRUNC, DEFAULT_FILE_MODE);
        if (fd > 0) descr.outfd = fd;
        else descr.good = handleopenerrno(output_filename);
    } else if (append_filename != NULL) {
        errno = 0;
        const int fd = open(append_filename, O_CREAT | O_WRONLY | O_APPEND, DEFAULT_FILE_MODE);
        if (fd > 0) descr.outfd = fd;
        else descr.good = handleopenerrno(append_filename);
    }

    return descr;
}

void
executeexternalcommand(char **argv, inoutdescriptors descr)
{
    pid_t child_pid = fork();
    if (child_pid == 0) {
        if (descr.infd != STDIN_FILENO) {
            const int newfd = dup2(descr.infd, STDIN_FILENO);
            assert(newfd == STDIN_FILENO);
        }
        if (descr.outfd != STDOUT_FILENO) {
            const int newfd = dup2(descr.outfd, STDOUT_FILENO);
            assert(newfd == STDOUT_FILENO);
        }

        execvp(argv[0], argv);
        switch (errno) {
            case ENOENT:
                fprintf(stderr, "%s: no such file or directory\n", argv[0]);
                break;
            case EACCES:
                fprintf(stderr, "%s: permission denied\n", argv[0]);
                break;
            default:
                fprintf(stderr, "%s: exec error\n", argv[0]);
                break;
        }
        exit(EXEC_FAILURE);
    } else if (child_pid < 0) {
        LOG_ERROR("fork() failed.");
    } else {
        waitpid(child_pid, NULL, 0);
    }
}

void
executecommand(command *pcmd)
{
    if (pcmd==NULL){
        // Do nothing.
        return;
    }

    execargs exec_args = getexecargsfromcommand(pcmd);
    inoutdescriptors descr = getinoutdescriptorsfromcommand(pcmd);
    builtin_pair *builtin_command = getbuiltincommand(exec_args.argv[0]);

    if (!descr.good) return;

    if (builtin_command != NULL)
        builtin_command->fun(exec_args.argv);
    else
        executeexternalcommand(exec_args.argv, descr);
}
