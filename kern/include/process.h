#ifndef _PROCESS_H_
#define _PROCESS_H_

#include <types.h>

#define MAX_PROCESSES 4


struct process {
	pid_t pid;
	struct thread* p_thread;

    // holds index of open files in OpenFileTable
    struct array* fd_table; //file descriptor table
};

// bootstraps initial process and process table
void process_bootstrap();
struct process * p_create();
void p_destroy(struct process *p);
void p_assign_thread(struct process * proc, struct thread * thread);


struct process_table;
struct process_table * pt_create();

#endif // _PROCESS_H_

