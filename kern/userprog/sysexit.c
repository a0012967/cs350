#include <syscall.h>
#include <process.h>

#if OPT_A2
    void sys__exit(int exitcode) {
        (void)exitcode;
        p_destroy();
    }
#endif /* OPT_A2 */
