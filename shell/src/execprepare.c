#include "execprepare.h"

#include "config.h"
#include "debugging.h"

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <assert.h>

#define MAX_ARGS ((MAX_LINE_LENGTH+1)/2)
static char* argv_pool[MAX_ARGS+1]; // The last one for terminating NULL

bool
islinevalid(pipelineseq * line)
{
    if (!line) return false;

    pipelineseq * ps = line;
    do {
        if (!ps || getpipelinelen(ps->pipeline) == -1)
            return false;
        ps = ps->next;
    } while (ps != line);

    return true;
}

bool
iscommandvalid(command * command)
{
    if (!command) return true; // Command empty, which is valid.
    if (!command->args) return false;
    if (!command->args->arg) return false;
    if (command->args->arg[0] == '\0') return false;
    return true;
}

// Returns -1 when the pipeline is invalid
int
getpipelinelen(pipeline * pipeline) {
    if (!pipeline) return -1;

    int pipeline_len = 0;
    int empty_commands_cnt = 0;

    commandseq * start = pipeline->commands;
    commandseq * cs = start;
    do {
        if (!cs || !iscommandvalid(cs->com))
            return -1;
        if (!cs->com)
            ++empty_commands_cnt;
        ++pipeline_len;
        cs = cs->next;
    } while (cs != start);

    if (pipeline_len > 1 && empty_commands_cnt >= 1)
        return -1;

    return pipeline_len;
}

// Returns pointers to statically allocated pool.
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
    descr.parent_descriptors = NULL;

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

