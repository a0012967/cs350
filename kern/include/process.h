#ifndef _PROCESS_H_
#define _PROCESS_H_

#include <types.h>

#define MAX_PROCESSES 4

struct process {
	pid_t pid;
	struct thread* p_thread;
    struct filetable* file_table; 
};

// bootstraps initial process. calls thread bootstrap and processtable bootstrap
void process_bootstrap();
// creates process and inserts it to processtable. also assigns pid inside
// Returns NULL on failure
struct process * p_create();
void p_destroy();
void p_assign_thread(struct process *p, struct thread *thread);

#endif // _PROCESS_H_

