#include <syscall.h>
#include <process.h>
#include <synch.h>

void sys__exit(int exitcode) {
	(void)exitcode;
    // struct process *curprocess = processtable_get(curthread->pid);
	/*curprocess->exitcode = exitcode;
	curprocess->has_exited = 1;
	cv_signal(curprocess->p_waitcv, curprocess->p_lock);
	thread_exit();*/
	p_destroy();
}
