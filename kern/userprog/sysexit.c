#include <syscall.h>
#include <thread.h>
#include <curprocess.h>
#include <process.h>
#include "opt-A2.h"

#if OPT_A2
    void sys__exit(int exitcode) {
        (void)exitcode;
        p_destroy(curprocess);
    }
#endif /* OPT_A2 */
