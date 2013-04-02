#ifndef _PROCESS_H_
#define _PROCESS_H_

#include <types.h>

#define MAX_PROCESSES 10

struct process {
    int has_exited; // 1 if this process has exited, 0 if not
    int exitcode; // the exit code of the process
    pid_t parentpid; // the pid of this process’s parent
    pid_t pid; // the pid of this process
    struct cv* p_waitcv; // condition variable in which to wait on
    struct lock* p_lock; // lock to be used for synchronization during wait
    struct thread* p_thread; // the thread that this process holds
    struct filetable* file_table; // table of files currently opened in the process
    struct array * p_childrenpids; // a list of all of this process’s children

    struct pagetable* pagetable;
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

