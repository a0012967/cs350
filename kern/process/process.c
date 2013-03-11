// TODO do something with the errs
// TODO change thread_create() in thread.c back to static after
//      there is no use for the test in main.c

#include <process.h>
#include <curprocess.h>
#include <processtable.h>
#include <array.h>
#include <linkedlist.h>
#include <lib.h>
#include <thread.h>
#include <curthread.h>
#include <filetable.h>
#include <array.h>

// global boolean, needed for opening console
// devices
int inprocessbootstrap;

// global reference to current process
struct process *curprocess;

struct process_table {
	struct array *process_list;
	struct linkedlist* freed_pids;
	pid_t nextpid;
};

void process_bootstrap() {
    // bootstrap processtable
    processtable_bootstrap();
    
    inprocessbootstrap = 1;
	struct process * p = p_create();
    if (p == NULL) {
        panic("PROCESS: Process bootstrap failed\n");
    }

    // bootstraps thread
	p->p_thread = thread_bootstrap();
    assert(p->p_thread != NULL);

    curprocess = p;
    
}

struct process * p_create() {
	struct process *p = kmalloc(sizeof(struct process));
	if (p == NULL)  {
        return NULL;
    }

    // instantiate file table
    p->file_table = ft_create();
    if (p->file_table == NULL) {
        // free allocated process cause it failed
        kfree(p);
        return NULL;
    }

    // insert process to process table
    int index = processtable_insert(p);
    p->pid = (pid_t)index;

	return p;
}

// Cause the current process to be destroyed
void p_destroy() {
    // make sure we're deleting the current process and current thread
    assert(curthread == curprocess->p_thread);

    // destroy filetable
    ft_destroy(curprocess->file_table);

    // free memory allocated to the process
    kfree(curprocess); // TODO: process scheduling?

    // exit the current thread
    thread_exit();
}

void p_assign_thread(struct process *p, struct thread *t) {
	p->p_thread = t;
}

