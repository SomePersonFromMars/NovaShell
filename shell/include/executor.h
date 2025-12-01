#ifndef _EXECUTOR_H_
#define _EXECUTOR_H_

#include "siparse.h"

#include <stdbool.h>
#include <sys/types.h>

typedef struct inoutdescriptors {
    int infd;
    int outfd;
    bool good;
    int *parent_descriptors; // Array ending with -1
} inoutdescriptors;

void executeline(pipelineseq * line);
pid_t executecommand(command *pcmd, inoutdescriptors suggested_descr, bool new_session);

#endif /* !_EXECUTOR_H_ */
