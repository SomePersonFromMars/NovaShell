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

static inoutdescriptors
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

// Execute with a suggested input and output file descriptors.
// Close the file descriptors once passed to the child.
// Returns the the child's pid if it has been created successfully, -1 otherwise.
pid_t
executeexternalcommand(char **argv, inoutdescriptors descr)
{
    const int infd = descr.infd;
    const int outfd = descr.outfd;

    pid_t child_pid = fork();
    if (child_pid == 0) {
        if (infd != STDIN_FILENO) {
            const int newfd = dup2(infd, STDIN_FILENO);
            assert(newfd == STDIN_FILENO);
        }
        if (outfd != STDOUT_FILENO) {
            const int newfd = dup2(outfd, STDOUT_FILENO);
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
        // Do nothing
    }

    if (infd != STDIN_FILENO) {
        const int r = close(infd);
        if (r != 0) LOG_ERROR("close() error.");
    }
    if (outfd != STDOUT_FILENO) {
        const int r = close(outfd);
        if (r != 0) LOG_ERROR("close() error.");
    }

    return child_pid;
}

// Execute with a suggested input and output file descriptors.
// Close the file descriptors once passed to the child.
// Returns the the child's pid if it has been created successfully, -1 otherwise.
// When a builtin command was run or the command was empty, 0 is returned.
pid_t
executecommand(command *pcmd, int infd, int outfd)
{
    if (pcmd==NULL){
        // Do nothing.
        return 0;
    }

    execargs exec_args = getexecargsfromcommand(pcmd);
    builtin_pair *builtin_command = getbuiltincommand(exec_args.argv[0]);
    inoutdescriptors descr;
    if (builtin_command) {
        // Redirections for builtin commands are undefined.
        descr.infd = infd = STDIN_FILENO;
        descr.outfd = outfd = STDOUT_FILENO;
        descr.good = true;
    } else {
        descr = getinoutdescriptorsfromcommand(pcmd);
    }

    if (!descr.good) return -1;
    // Assure I/O redirections precedence.
    if (descr.infd == STDIN_FILENO) // If unset.
        descr.infd = infd;
    else if (infd != STDIN_FILENO) { // If set, ignore it.
        const int r = close(infd);
        if (r != 0) LOG_ERROR("close() error.");
    }
    if (descr.outfd == STDOUT_FILENO) // If unset.
        descr.outfd = outfd;
    else if (outfd != STDOUT_FILENO) { // If set, ignore it.
        const int r = close(outfd);
        if (r != 0) LOG_ERROR("close() error.");
    }

    if (builtin_command != NULL) {
        builtin_command->fun(exec_args.argv);
        return 0;
    } else {
        return executeexternalcommand(exec_args.argv, descr);
    }
}

static bool
iscommandvalid(command * command)
{
    if (!command) return true; // Command empty, which is valid.
    if (!command->args) return false;
    if (!command->args->arg) return false;
    if (command->args->arg[0] == '\0') return false;
    return true;
}

static bool
ispipelinevalid(pipeline * pipeline) {
    if (!pipeline) return false;

    int pipeline_len = 0;
    int empty_commands_cnt = 0;

    commandseq * start = pipeline->commands;
    commandseq * cs = start;
    do {
        if (!cs || !iscommandvalid(cs->com))
            return false;
        if (!cs->com)
            ++empty_commands_cnt;
        ++pipeline_len;
        cs = cs->next;
    } while (cs != start);

    if (pipeline_len > 1 && empty_commands_cnt >= 1)
        return false;

    return true;
}

static bool
islinevalid(pipelineseq * line)
{
    if (!line) return false;

    pipelineseq * ps = line;
    do {
        if (!ps || !ispipelinevalid(ps->pipeline))
            return false;
        ps = ps->next;
    } while (ps != line);

    return true;
}

void
executepipeline(pipeline * pipeline)
{
    assert(pipeline);
    if (pipeline->flags & INBACKGROUND) {
        LOG_ERROR("Background execution unimplemented.");
        return;
    }

    int children_cnt = 0;

    int infd = STDIN_FILENO;
    commandseq * start = pipeline->commands;
    commandseq * cs = start;
    do {
        assert(cs && iscommandvalid(cs->com));

        int new_infd = -1;
        int outfd = STDOUT_FILENO;

        const bool last_com = (cs->next == start);
        if (!last_com) {
            int pipefd[2];
            pipe(pipefd);
            new_infd = pipefd[0];
            outfd = pipefd[1];
        }

        assert(infd != -1);
        const pid_t child_pid = executecommand(cs->com, infd, outfd);
        if (child_pid > 0) ++children_cnt;

        infd = new_infd;
        cs = cs->next;
    } while (cs != start);

    while (children_cnt--) {
        const pid_t r = wait(NULL);
        assert(r != -1);
    }
}

void
executeline(pipelineseq * line)
{
	if (!islinevalid(line)) {
		printf("%s\n", SYNTAX_ERROR_STR);
		return;
	}

	pipelineseq * ps = line;
	do {
        executepipeline(ps->pipeline);
		ps = ps->next;
	} while (ps != line);
}
