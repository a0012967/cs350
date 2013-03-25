#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#include <types.h>
#include "opt-A2.h"

/*
 * Prototypes for IN-KERNEL entry points for system call implementations.
 */
struct trapframe;
int sys_reboot(int code);

#if OPT_A2

/********************
 * Syscall functions
 ********************/

    int sys_open(const char *filename, int flags, int *err);
    int sys_close(int fd, int *err);
    int sys_read(int fd, userptr_t buf, size_t buflen,  int *retval);
    int sys_write(int fd, void *buf, size_t buflen, int *err);
    pid_t sys_fork(struct trapframe *tf, int *err); 
    int sys_waitpid(pid_t pid, int *status, int options, int *err);
    pid_t sys_getpid(void);
    int sys_execv(const char *program, char **args, int *err);
    void sys__exit(int exitcode);


/********************
 * Helper functions
 ********************/

    int buffer_check(void *buf, size_t buflen);

#endif /* OPT_A2 */


#endif /* _SYSCALL_H_ */
