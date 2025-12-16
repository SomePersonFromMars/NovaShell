#ifndef _EXECUTOR_H_
#define _EXECUTOR_H_

#include "execprepare.h"
#include "siparse.h"

#include <stdbool.h>
#include <sys/types.h>

void executeline(pipelineseq * line);
pid_t executecommand(command *pcmd, inoutdescriptors suggested_descr, bool new_session);

#endif /* !_EXECUTOR_H_ */
