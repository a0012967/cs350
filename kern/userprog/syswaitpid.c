#include <syscall.h>
#include <thread.h>
#include <curthread.h>
#include <process.h>
#include <processtable.h>
#include <synch.h>
#include <lib.h>
#include <types.h>
#include <kern/errno.h>
#include <vm.h>

int sys_waitpid(pid_t pid, int *status, int options, int* err) {
	// make sure no options are passed
    if (options != 0) {
        *err = EINVAL;
        return -1;
    }

	// make sure status pointer is not null
    if (status == NULL) {
        *err = EFAULT;
        return -1;
    }

	// check if the buffer is valid
    int dest;
	int ptr_check = copyin((userptr_t)status, &dest, sizeof(status));
	if (ptr_check !=0) {
		*err = ptr_check;
		return -1;
	}

	// make sure pointer is aligned
	if ((int)status % 4 != 0){
		*err = EFAULT;
		return -1;
	}
    
    struct process * proc = processtable_get(pid);
    if (proc == NULL) {
        *err = EINVAL;
        return -1;
    }

    lock_acquire(proc->p_lock); 
	    if (proc->parentpid != curthread->pid) {
	        *err = EINVAL;
            lock_release(proc->p_lock);
            return -1;
	    }
	
        while (!proc->has_exited) {
            cv_wait(proc->p_waitcv, proc->p_lock);
        }

        *status = proc->exitcode;

        // remove the child from the proc table
        processtable_remove(pid);
    lock_release(proc->p_lock);

    // free the child
    p_destroy_at(proc);

	return pid;
}
