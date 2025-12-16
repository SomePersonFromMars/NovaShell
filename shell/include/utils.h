#ifndef _UTILS_H_
#define _UTILS_H_

#include "siparse.h"
#include <limits.h>

#define NOP ((void)0)
#define INVALID_ID SIZE_MAX

void printcommand(command *, int);
void printpipeline(pipeline *, int);
void printparsedline(pipelineseq *);

command * pickfirstcommand(pipelineseq *);

#endif /* !_UTILS_H_ */
