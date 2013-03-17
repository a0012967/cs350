// TODO do something with the errs
// TODO change thread_create() in thread.c back to static after
//      there is no use for the test in main.c

#include <process.h>
#include <processtable.h>
#include <array.h>
#include <linkedlist.h>
#include <lib.h>
#include <thread.h>
#include <curthread.h>
#include <filetable.h>
#include <synch.h>

struct process_table {
	struct array *process_list;
	struct linkedlist* freed_pids;
	pid_t nextpid;
};

void process_bootstrap() {
    // bootstrap processtable
    processtable_bootstrap();
	
    struct process * p = p_create();
    if (p == NULL) {
        panic("PROCESS: Process bootstrap failed\n");
    }

    struct thread *t = thread_bootstrap();
    assert(t != NULL);

    p_assign_thread(p, t);
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

    // initiate condition variable for waitpid
    p->p_waitcv = cv_create("proc_cv");
    if (p->p_waitcv == NULL) {
        // free allocated process cause it failed
        ft_destroy(p->file_table);
        kfree(p);
        return NULL;
    }

    // initiate its lock
    p->p_lock = lock_create("proc_lock");
    if (p->p_lock == NULL) {
        // free allocated process cause it failed
        ft_destroy(p->file_table);
        cv_destroy(p->p_waitcv);
        kfree(p);
        return NULL;
    }
    
    // initiate its array of children
    p->p_childrenpids = array_create();
    if (p->p_childrenpids == NULL) {
        // free allocated process cause it failed
        ft_destroy(p->file_table);
        cv_destroy(p->p_waitcv);
        lock_destroy(p->p_lock);
        kfree(p);
        return NULL;
    }

    // signify process hasn't exited yet
    p->has_exited = 0;
    p->exitcode = -1;

    // set parent pid to 0 for now -> this value needs to be
    // set explicitly when doing a fork
    p->parentpid = 0;

    // insert process to process table
    int err = 0;
    int index = processtable_insert(p, &err);
    if (index == -1) {
        ft_destroy(p->file_table);
        cv_destroy(p->p_waitcv);
        lock_destroy(p->p_lock);
        kfree(p);
        return NULL;
    }
    p->pid = (pid_t)index;

	return p;
}

// Cause the current process to be destroyed
void p_destroy() {
    struct process *curprocess = processtable_get(curthread->pid);

    // destroy filetable
    ft_destroy(curprocess->file_table);

    // destroy synch primitives
    lock_destroy(curprocess->p_lock);
    cv_destroy(curprocess->p_waitcv);

    // destroy array of children
    array_destroy(curprocess->p_childrenpids);

    // free memory allocated to the process
    kfree(curprocess);

    // exit the current thread
    thread_exit();
}

// destroy the specified process
void p_destroy_at(struct process * p) {
    // destroy filetable
    ft_destroy(p->file_table);

    // destroy synch primitives
    lock_destroy(p->p_lock);
    cv_destroy(p->p_waitcv);
    
    // destroy array of children
    array_destroy(p->p_childrenpids);

    kfree(p);
}

void kill_process(int exitcode) {
    struct process *curprocess = processtable_get(curthread->pid);
	lock_acquire (curprocess->p_lock);

		curprocess->exitcode = exitcode;
		curprocess->has_exited = 1;
	    cv_signal(curprocess->p_waitcv, curprocess->p_lock);

		// TODO fix process children
		/*// set all of the curprocess children to have a parent of pid 0
		// (signifies that the parent is dead)
		int i;
		pid_t childpid;
		struct process * curprocess_child;
		for (i = 0; i < array_getnum(curprocess->p_childrenpids); i++) {
			childpid = array_getguy(curprocess->p_childrenpids, i);
			curprocess_child = processtable_get(childpid);
			curprocess_child->parentpid = 0;

		}*/
    lock_release (curprocess->p_lock);
	
	// TODO fix process children
	// if the parent is dead, free memory
	// if not, the parent will take care of freeing memory
	//if (curprocess->parentpid == 0) {
	//	processtable_remove(curprocess->pid);
	//	p_destroy();
	//}
	//else {
		thread_exit();
	//}
}

void p_assign_thread(struct process *p, struct thread *t) {
	p->p_thread = t;
    t->pid = p->pid;
}

