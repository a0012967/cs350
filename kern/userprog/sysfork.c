#include <syscall.h>
#include <process.h>
#include <curprocess.h>
#include <processtable.h>
#include <lib.h>

pid_t sys_fork(int *err) {
    assert(err != NULL);
    assert(*err == 0);

    return 0;
}
