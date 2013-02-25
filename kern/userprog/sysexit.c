#include <syscall.h>

#if OPT_A2
    void sys__exit(int exitcode) {
        (void)exitcode;
    }
#endif /* OPT_A2 */
