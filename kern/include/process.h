#ifndef _PROCESS_H_
#define _PROCESS_H_

#include <types.h>

#define MAX_PROCESSES 10

struct process {
    int has_exited; // TODO init to 0
    int exitcode; // TODO init to 0
    pid_t parentpid; // TODO init on fork -> should we keep this?
	pid_t pid;
	struct cv* p_waitcv; // TODO malloc on create, free on destroy
	struct lock* p_lock; // TODO malloc on create, free on destroy
	struct thread* p_thread;
    struct filetable* file_table; 
};


/* global flag needed for opening console files 
 * has value 1 when in process bootstrap
 *  set to 0 at end of process bootstrap */
int inprocessbootstrap;

// bootstraps initial process. calls thread bootstrap and processtable bootstrap
void process_bootstrap();
// creates process and inserts it to processtable. also assigns pid inside
// Returns NULL on failure
struct process * p_create();
void p_destroy();
void p_destroy_at(struct process *p);
void p_assign_thread(struct process *p, struct thread *thread);

#endif // _PROCESS_H_

