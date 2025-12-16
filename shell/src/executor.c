#include "executor.h"

#include "subprocessesmanager.h"
#include "builtins.h"
#include "config.h"
#include "siparse.h"
#include "debugging.h"

#include <errno.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>

#define DEFAULT_FILE_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH)
#define MAX_ARGS ((MAX_LINE_LENGTH+1)/2)

typedef struct execargs {
    size_t argc;
    char** argv;
} execargs;

static char* argv_pool[MAX_ARGS+1]; // The last one for terminating NULL

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

static inline void
closeifdifferentandhandleerror(int fd, int noclosefd)
{
    if (fd != noclosefd) {
        const int r = close(fd);
        if (r != 0) LOG_ERROR("close() error.");
    }
}

// Execute with a suggested input and output file descriptors.
// Close the file descriptors once passed to the child.
// Returns the the child's pid if it has been created successfully, -1 otherwise.
pid_t
executeexternalcommand(char **argv, inoutdescriptors descr, bool new_session)
{
    const int infd = descr.infd;
    const int outfd = descr.outfd;

    pid_t child_pid = fork();
    if (child_pid == 0) {
        revertdefaultsignalhandlersandmasks();

        if (new_session && setsid() == -1)
            LOG_ERROR("Error while creating a new session");

        while (descr.parent_descriptors && *descr.parent_descriptors != -1) {
            closeifdifferentandhandleerror(*descr.parent_descriptors, 0);
            ++descr.parent_descriptors;
        }
        if (infd != STDIN_FILENO) {
            const int newfd = dup2(infd, STDIN_FILENO);
            assert(newfd == STDIN_FILENO);
            closeifdifferentandhandleerror(infd, STDIN_FILENO);
        }
        if (outfd != STDOUT_FILENO) {
            const int newfd = dup2(outfd, STDOUT_FILENO);
            assert(newfd == STDOUT_FILENO);
            closeifdifferentandhandleerror(outfd, STDIN_FILENO);
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

    closeifdifferentandhandleerror(infd, STDIN_FILENO);
    closeifdifferentandhandleerror(outfd, STDOUT_FILENO);

    return child_pid;
}

// Execute with a suggested input and output file descriptors.
// Close the file descriptors once passed to the child.
// Returns the the child's pid if it has been created successfully, -1 otherwise.
// When a builtin command was run or the command was empty, 0 is returned.
pid_t
executecommand(command *pcmd, inoutdescriptors suggested_descr, bool new_session)
{
    if (!suggested_descr.good) return -1;
    if (pcmd==NULL){
        // Do nothing.
        return 0;
    }

    execargs exec_args = getexecargsfromcommand(pcmd);
    builtin_pair *builtin_command = getbuiltincommand(exec_args.argv[0]);
    inoutdescriptors descr;
    if (builtin_command) {
        // Redirections for builtin commands are undefined.
        closeifdifferentandhandleerror(suggested_descr.infd, STDIN_FILENO);
        closeifdifferentandhandleerror(suggested_descr.outfd, STDOUT_FILENO);
        descr.infd = suggested_descr.infd = STDIN_FILENO;
        descr.outfd = suggested_descr.outfd = STDOUT_FILENO;
        descr.good = true;
        descr.parent_descriptors = NULL;
    } else {
        descr = getinoutdescriptorsfromcommand(pcmd);
    }

    if (!descr.good) return -1;
    // Assure I/O redirections precedence.
    if (descr.infd == STDIN_FILENO) // If unset, replace it.
        descr.infd = suggested_descr.infd;
    else closeifdifferentandhandleerror(suggested_descr.infd, STDIN_FILENO);

    if (descr.outfd == STDOUT_FILENO) // If unset, replace it.
        descr.outfd = suggested_descr.outfd;
    else closeifdifferentandhandleerror(suggested_descr.outfd, STDOUT_FILENO);

    assert(!descr.parent_descriptors);
    descr.parent_descriptors = suggested_descr.parent_descriptors;

    if (builtin_command != NULL) {
        assert(!new_session);
        builtin_command->fun(exec_args.argv);
        return 0;
    } else {
        return executeexternalcommand(exec_args.argv, descr, new_session);
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

// Returns -1 when the pipeline is invalid
static int
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

static bool
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

void
executepipeline(pipeline * pipeline)
{
    assert(pipeline);
    const bool background_pipeline = (pipeline->flags & INBACKGROUND);

    int pipeline_len = getpipelinelen(pipeline);
    assert(pipeline_len >= 0);
    const int fg_children_buffer_size = (background_pipeline ? 0 : pipeline_len);

    pid_t fg_children[fg_children_buffer_size+1];
    fg_children[fg_children_buffer_size] = 0;

    int children_cnt = 0;
    int infd = STDIN_FILENO;
    commandseq * start = pipeline->commands;
    commandseq * cs = start;

    LOG_INFO("Before block.");
    LOG_EXPR_INT(issignalblocked(SIGCHLD));
    sigset_t original_set = blocksigchld();
    LOG_INFO("After block.");

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
        {
            inoutdescriptors descr;
            descr.infd = infd;
            descr.outfd = outfd;
            descr.good = true;
            int parent_descriptors[] = { new_infd, -1 };
            descr.parent_descriptors = parent_descriptors;
            const pid_t child_pid = executecommand(cs->com, descr, background_pipeline);
            if (!background_pipeline)
                fg_children[children_cnt] = child_pid;

            if (child_pid > 0) ++children_cnt;
        }

        infd = new_infd;
        cs = cs->next;
    } while (cs != start);

    const int fg_children_cnt = (background_pipeline ? 0 : children_cnt);
    setforegroundprocesses(fg_children, fg_children_cnt);
    #ifdef DEBUG_PRINT
        for (int i = 0; i < fg_children_cnt; ++i)
            LOG_EXPR_INT(fg_children[i]);        
    #endif
    waitforforegroundprocessestofinish(&original_set);
    unblocksigchld(&original_set);
}

void
executeline(pipelineseq * line)
{
	if (!islinevalid(line)) {
		printf("%s\n", SYNTAX_ERROR_STR);
		return;
	}

    LOG_OPEN_FDS_COUNT();
	pipelineseq * ps = line;
	do {
        executepipeline(ps->pipeline);
		ps = ps->next;
	} while (ps != line);
}
