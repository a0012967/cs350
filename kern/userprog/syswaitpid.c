#include <syscall.h>
#include <curprocess.h>
#include <curthread.h>
#include <thread.h>
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
    struct thread * th = proc->p_thread;
    
    if (proc == NULL) {
        *err = EINVAL; // TODO correct err val?
        return -1;
    }
    
    lock_acquire(proc->p_lock); 

	    if (th->parentpid != curthread->pid) {
	        *err = EINVAL; // TODO correct err val?
            lock_release(th->t_lock);
            return -1;
	    }
	
        while (!th->has_exited) {
            cv_wait(th->t_waitcv, th->t_lock);
        }
        *status = th->exitcode;
        
        // remove the child from the proc table
        processtable_remove(pid);
    lock_release(th->t_lock);
    
    // free the child
    p_destroy_at(proc);
    
	return pid;
}
