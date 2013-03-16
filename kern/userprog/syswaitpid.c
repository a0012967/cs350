#include <syscall.h>
#include <curprocess.h>
#include <process.h>
#include <synch.h>
#include <kern/errno.h>
#include <processtable.h>

/*
Is the status pointer properly aligned (by 4) ?
Is the status pointer a valid pointer anyway (NULL, point to kernel, …)?
Is options valid? (More flags than WNOHANG | WUNTRACED )
Does the waited pid exist/valid?
If exist, are we allowed to wait it ? (Is it our child?)
*/

int sys_waitpid(pid_t pid, int *status, int options, int* err) {

    if (options != 0) {
        *err = EINVAL;
        return -1;
    }
    
    if (status == NULL) {
        *err = EFAULT;
        return -1;
    }
	
    struct process * proc = processtable_get(pid);
    
    if (proc == NULL) {
        *err = EINVAL; // TODO correct err val?
        return -1;
    }
    
   lock_acquire(proc->p_lock); 
	    if (proc->parentpid != curprocess->pid) {
	        *err = EINVAL; // TODO correct err val?
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
