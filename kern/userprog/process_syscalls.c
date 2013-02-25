int sys_getpid(void) {
	return curthread->t_pid;
}

int sys_write(int filehandle, const void *buf, size_t size) {
	struct uio uio_write;
	char * write_buffer = (char*)kmalloc(size);
	
	mk_kuio(&uio_write, (void*)write_buffer, size, /*offset*/, UIO_WRITE);
	
}


int sys_waitpid(pid_t pid, int *returncode, int flags) {
	
	// to-do: test arguments, eg. flags > 0

	
	lock_acquire(array_getguy(proc_tab, pid)->tdExitLock);
	cv_wait(array_getguy(proc_tab, pid)->tdExitCV, array_getguy(proc_tab, pid)->tdExitLock);
	lock_release(array_getguy(proc_tab, pid)->tdExitLock);
	*returncode = array_getguy(proc_tab, pid)->exitcode;
	return pid;
}
