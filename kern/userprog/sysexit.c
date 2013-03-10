#include <syscall.h>
#include <process.h>

void sys__exit(int exitcode) {
	(void)exitcode;
	p_destroy();
}
