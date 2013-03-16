#include <syscall.h>
#include <curprocess.h>
#include <curthread.h>
#include <process.h>
#include <synch.h>

void sys__exit(int exitcode) {
	(void)exitcode;
	lock_acquire (curthread->t_lock);
		curprocess->exitcode = exitcode;
		curprocess->has_exited = 1;
	    cv_signal(curthread->t_waitcv, curthread->t_lock);
    lock_release (curthread->t_lock);
	thread_exit();
	//p_destroy();
}
