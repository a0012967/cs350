#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#include <types.h>
#include "opt-A2.h"

/*
 * Prototypes for IN-KERNEL entry points for system call implementations.
 */

int sys_reboot(int code);

#if OPT_A2
    int sys_open(const char *filename, int flags);
    int sys_close(int fd);
    int sys_read(int fd, void *buf, size_t buflen);
    void sys__exit(int exitcode);
    pid_t sys_fork(void);
#endif /* OPT_A2 */


#endif /* _SYSCALL_H_ */
