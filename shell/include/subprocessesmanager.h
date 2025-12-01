#ifndef _SUBPROCESSESMANAGER_H_
#define _SUBPROCESSESMANAGER_H_

#include <sys/types.h>

#define MAX_CHILD_STATUSES_CNT 64

void setupsubprocesseswatcherandsignals(void);
// `new_foreground_processes_list` is a 0-ended array
void setforegroundprocesses(pid_t new_foreground_processes_list[], int new_running_fg_children_count);
void printpendingbgchildrenstatuses(void);
void revertdefaultsignalhandlers(void);
sigset_t blocksigchld(void);
void unblocksigchld(const sigset_t *original_set);
void waitforforegroundprocessestofinish(void);

#endif /* !_SUBPROCESSESMANAGER_H_ */
