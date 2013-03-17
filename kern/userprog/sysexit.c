#include <syscall.h>
#include <process.h>

void sys__exit(int exitcode) {
    kill_process(exitcode);
}
