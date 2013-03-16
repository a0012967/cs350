#include <syscall.h>
#include <process.h>
#include <curprocess.h>
#include <lib.h>

pid_t sys_getpid() {
    return curprocess->pid;
}
