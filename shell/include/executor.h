#ifndef _EXECUTOR_H_
#define _EXECUTOR_H_

#include "siparse.h"

#include <sys/types.h>

void executeline(pipelineseq * line);
pid_t executecommand(command *pcmd, int infd, int outfd);

#endif /* !_EXECUTOR_H_ */
