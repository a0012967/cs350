// TODO do something with the errs
// TODO change thread_create() in thread.c back to static after
//      there is no use for the test in main.c

#include <process.h>
#include <array.h>
#include <linkedlist.h>
#include <types.h>
#include <lib.h>
#include <thread.h>

struct process_table {
	struct array *process_list;
	pid_t nextpid;
	struct linkedlist* freed_pids;
};

struct process {
	pid_t pid;
	struct thread* thread;
};

static struct process_table * proc_tab;
int max_processes = 4; // includes PID 0

struct process * process_create() {
	struct process * proc;
	proc = kmalloc(sizeof(struct process));
	if (proc == NULL) return NULL;
	return proc;
}

void process_assign_thread(struct process * proc, struct thread * thread) {
	proc->thread = thread;
}

struct process_table * process_table_create() {
	struct process_table *p_t;
	p_t = kmalloc(sizeof(struct process_table));
	p_t->process_list = array_create();
	p_t->freed_pids = ll_create();
	if (p_t == NULL || p_t->process_list == NULL ||
			p_t->freed_pids == NULL) return NULL;
	p_t->nextpid = 0;
	return p_t;
}

struct process * process_bootstrap(void) {
	struct process * proc;
	proc_tab = process_table_create();
	proc = process_create();
	proc->thread = thread_bootstrap();
	int err = assign_pid(proc);
}


// Finds an available pid for use to assign to the supplied process.
int assign_pid (struct process *proc) {
	if (array_getnum(proc_tab->process_list) == max_processes) {
		// if all of the pids are completely used up
		if (ll_empty(proc_tab->freed_pids)) {
			return -1; // return EAGAIN
		}
		// there are no pids available in end of the process_list, so
		// use the first pid available on the freed_pid list
		proc->pid = *((int*)ll_front(proc_tab->freed_pids));
		kfree(ll_pop_front(proc_tab->freed_pids));
		array_setguy(proc_tab->process_list, proc->pid, proc);
	} else {
		int err = array_preallocate(proc_tab->process_list, proc_tab->nextpid+1);
		if (err == 4) { // err == ENOMEM
			return -1; // return ENOMEM
		}
		// there are still slots available at the end of the process_list
		// so use nextpid as the pid
		proc->pid = proc_tab->nextpid;
		array_add(proc_tab->process_list, proc);
		proc_tab->nextpid++;
	}
	return 0;
}

// Remove the process with the supplied pid from the process list,
// by replacing that spot with a NULL. This is to maintain the pids
// for the rest of the list. Also adds this pid as an available
// pid for use in the freed pids linked list.
void remove_pid (pid_t pid) {
	int * free_pid = kmalloc(sizeof(int));
	*free_pid = pid;
	ll_push_back(proc_tab->freed_pids, free_pid);
	array_setguy(proc_tab->process_list, pid, NULL);
}


// print the process table for debugging
void print_proc_table () {
	int i = 0;
	kprintf("table size: %d\n", array_getnum(proc_tab->process_list));
	for (i = 0; i <= array_getnum(proc_tab->process_list)-1; i++) {
		if (array_getguy(proc_tab->process_list, i) != NULL) {
			kprintf("pid: %d\nthread name:", ((struct process *)(array_getguy(proc_tab->process_list, i)))->pid);
			kprintf(((struct process *)(array_getguy(proc_tab->process_list, i)))->thread->t_name);
			kprintf("\n");
		}
	}
	kprintf("\n");
	ll_print(proc_tab->freed_pids);
}



