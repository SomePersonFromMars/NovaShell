#ifndef _EXEC_PREPARE_H_
#define _EXEC_PREPARE_H_

#include "siparse.h"

#include <stddef.h>
#include <stdbool.h>

typedef struct execargs {
    size_t argc;
    char** argv;
} execargs;

typedef struct inoutdescriptors {
    int infd;
    int outfd;
    bool good;
    int *parent_descriptors; // Array ending with -1
} inoutdescriptors;

bool islinevalid(pipelineseq * line);
bool iscommandvalid(command * command);
int getpipelinelen(pipeline * pipeline);
execargs getexecargsfromcommand(command *pcmd);
inoutdescriptors getinoutdescriptorsfromcommand(command *pcmd);

#endif /* !_EXEC_PREPARE_H_ */
