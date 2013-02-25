#include <syscall.h>

#if OPT_A2
    pid_t sys_fork(void) {
        return 0;
    }
#endif /* OPT_A2 */
