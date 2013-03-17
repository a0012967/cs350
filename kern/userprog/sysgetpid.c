#include <syscall.h>
#include <curthread.h>
#include <thread.h>

pid_t sys_getpid() {
    return curthread->pid;
}
