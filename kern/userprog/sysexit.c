#include <syscall.h>
#include <curthread.h>
#include <thread.h>
#include <process.h>
#include <processtable.h>
#include <synch.h>

void sys__exit(int exitcode) {
    struct process *curprocess = processtable_get(curthread->pid);
	lock_acquire (curprocess->p_lock);
		curprocess->exitcode = exitcode;
		curprocess->has_exited = 1;
	    cv_signal(curprocess->p_waitcv, curprocess->p_lock);
    lock_release (curprocess->p_lock);
	thread_exit();
	//p_destroy();
}
