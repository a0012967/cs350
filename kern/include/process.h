#ifndef _PROCESS_H_
#define _PROCESS_H_

int max_processes;

struct process_table {
	struct array *process_list;
	pid_t nextpid;
	struct linkedlist* freed_pids;
}

struct process {
	pid_t pid;
	struct thread* td;
}

#endif _PROCESS_H_

// to-do: rename create_process to process_create;
// process_table = proc_tab

struct process * process_bootstrap(void) {
	// to-do: assign a new thread to the current process
	struct process * proc;
	
	proc_tab = process_table_create();
	
	proc = process_create();
	
	
	array_setguy(proc_tab->process_list, nextpid, proc);
}

struct process * process_create() {
	struct process * proc
	proc = (struct process *) kmalloc(sizeof(struct process));
	return proc;
}

struct process_table * process_table_create() {
	struct process_table *p_t;
	p_t = (struct process_table *) kmalloc(sizeof(struct process_table));
	if (p_t == NULL) {
		panic("Cannot create process table\n");
	}
	p_t->process_list = array_create();
	if (p_t->process_list == NULL) {
		panic("Cannot create process list array\n");
	}
	p_t->freed_pids = ll_create();
	if (p_t->freed_list == NULL) {
		panic("Cannot create freed PID linked list\n");
	}	
	p_t->nextpid = 1;
	return p_t;
}

int assign_pid (struct process *proc) {
	if (array_getnum(proc_tab->process_list) == max_processes) {
		// if all of the pids are completely used up
		if (ll_empty(proc_tab->freed_pids)) {
			return -1; // return EAGAIN
		}
		// there are no pids available in end of the process_list, so
		// use the first pid available on the freed_pid list
		proc->pid = ll_pop_front(freed_pids);
		array_setguy(proc_tab->process_list, proc->pid, proc);
	} else {
		err = array_preallocate(proc_tab->process_list, proc_tab->nextpid+1);
		if (err == 4) { // err == ENOMEM
			return -1; // return ENOMEM
		}
		// there are still slots available at the end of the process_list
		// so use nextpid as the pid
		proc->pid = proc_tab->nextpid;
		array_add(proc_tab->proc_list, proc);
		proc_tab->nextpid++;
	}
	return 0;
}

void remove_pid (pid_t pid) {
	ll_push_back(proc_tab->freed_pids, &pid);
	array_setguy(proc_tab->process_list, pid, NULL);
}

