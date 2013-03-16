#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#include <types.h>
#include "opt-A2.h"

/*
 * Prototypes for IN-KERNEL entry points for system call implementations.
 */

// forward decls
struct trapframe;

int sys_reboot(int code);

#if OPT_A2
/********************
 * Syscall functions
 ********************/
    int sys_open(const char *filename, int flags, int *err);
    int sys_close(int fd, int *err);
    int sys_read(int fd, void *buf, size_t buflen,  int *retval);
    void sys__exit(int exitcode);
    // we pass the trap frame to fork so we can copy it
    pid_t sys_fork(struct trapframe *tf, int *err);
    pid_t sys_getpid(void);

/********************
 * Helper functions
 ********************/
    int buffer_check(void *buf, size_t buflen);
#endif /* OPT_A2 */


#endif /* _SYSCALL_H_ */
