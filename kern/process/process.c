// TODO do something with the errs
// TODO change thread_create() in thread.c back to static after
//      there is no use for the test in main.c

#include <process.h>
#include <curprocess.h>
#include <array.h>
#include <linkedlist.h>
#include <lib.h>
#include <thread.h>
#include <curthread.h>
#include <filetable.h>
#include <array.h>

/*****************************
 * Forward Declarations
 * **************************/
int assign_pid (struct process *proc);

struct process *curprocess;

struct process_table {
	struct array *process_list;
	struct linkedlist* freed_pids;
	pid_t nextpid;
};

static struct process_table *p_table;
// TODO: uncomment this
// static struct lock *pt_lock; // process table lock

/*****************************
 * PROCESS STUFF
 * **************************/

void process_bootstrap() {
	struct process * p;
	p_table = pt_create();
	p = p_create();
    if (p == NULL) {
        panic("process: Process bootstrap failed\n");
    }
	p->p_thread = thread_bootstrap();
	int err = assign_pid(p);
    // TODO: make use of err
    (void)err;

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

/*****************************
 * PROCESS TABLE STUFF
 * **************************/

struct process_table *pt_create() {
	struct process_table *p_t;
	p_t = kmalloc(sizeof(struct process_table));
    if (p_t == NULL) {
        panic("process table: Cannot create process table\n");
    }

	p_t->process_list = array_create();
    if (p_t->process_list == NULL) {
        panic("process table: Cannot create process_list\n");
    }

	p_t->freed_pids = ll_create();
    if (p_t->freed_pids == NULL) {
        panic("process table: Cannot create freed_pids\n");
    }

	p_t->nextpid = 0;
	return p_t;
}

// Finds an available pid for use to assign to the supplied process.
int assign_pid (struct process *proc) {
    // TODO: refactor later and apply locks
	if (array_getnum(p_table->process_list) == MAX_PROCESSES) {
		// if all of the pids are completely used up
		if (ll_empty(p_table->freed_pids)) {
			return -1; // return EAGAIN
		}
		// there are no pids available in end of the process_list, so
		// use the first pid available on the freed_pid list
		proc->pid = *((int*)ll_front(p_table->freed_pids));
		kfree(ll_pop_front(p_table->freed_pids));
		array_setguy(p_table->process_list, proc->pid, proc);
	} else {
		int err = array_preallocate(p_table->process_list, p_table->nextpid+1);
		if (err == 4) { // err == ENOMEM
			return -1; // return ENOMEM
		}
		// there are still slots available at the end of the process_list
		// so use nextpid as the pid
		proc->pid = p_table->nextpid;
		array_add(p_table->process_list, proc);
		p_table->nextpid++;
	}
	return 0;
}

// Remove the process with the supplied pid from the process list,
// by replacing that spot with a NULL. This is to maintain the pids
// for the rest of the list. Also adds this pid as an available
// pid for use in the freed pids linked list.
void remove_pid (pid_t pid) {
    // TODO: refactor later and apply locks
	int * free_pid = kmalloc(sizeof(int));
	*free_pid = pid;
	ll_push_back(p_table->freed_pids, free_pid);
	array_setguy(p_table->process_list, pid, NULL);
}


// print the process table for debugging
void print_proc_table() {
    /*
	int i = 0;
	kprintf("table size: %d\n", array_getnum(p_table->process_list));
	for (i = 0; i <= array_getnum(p_table->process_list)-1; i++) {
		if (array_getguy(p_table->process_list, i) != NULL) {
			kprintf("pid: %d\nthread name:", ((struct process *)(array_getguy(p_table->process_list, i)))->pid);
			kprintf(((struct process *)(array_getguy(p_table->process_list, i)))->thread->t_name);
			kprintf("\n");
		}
	}
	kprintf("\n");
	ll_print(p_table->freed_pids);
    */
}



