#include "executor.h"

#include "execprepare.h"
#include "subprocessesmanager.h"
#include "builtins.h"
#include "config.h"
#include "debugging.h"

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>

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
