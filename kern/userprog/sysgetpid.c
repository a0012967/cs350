#include <syscall.h>
#include <curprocess.h>
#include <process.h>

int sys_getpid(void) {
	return curprocess->pid;
}
