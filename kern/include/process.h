#ifndef _PROCESS_H_
#define _PROCESS_H_

#include <types.h>

#define MAX_PROCESSES 10

struct process {
    int has_exited;
    int exitcode; 
    pid_t parentpid;
	pid_t pid;
	struct cv* p_waitcv; 
	struct lock* p_lock; 
	struct thread* p_thread;
    struct filetable* file_table; 
    struct array * p_childrenpids; 
};

// bootstraps initial process. calls thread bootstrap and processtable bootstrap
void process_bootstrap();
// creates process and inserts it to processtable. also assigns pid inside
// Returns NULL on failure
struct process * p_create();
void kill_process(int exitcode);
void p_destroy();
void p_destroy_at(struct process *p);
void p_assign_thread(struct process *p, struct thread *thread);

#endif // _PROCESS_H_

