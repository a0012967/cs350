#include <syscall.h>
#include <vfs.h>
#include <file.h>
#include <filetable.h>
#include <curthread.h>
#include <thread.h>
#include <process.h>
#include <vnode.h>
#include <lib.h>
#include <types.h>
#include <kern/errno.h>

int sys_close(int fd, int *err) {
    assert(err != NULL);
    assert(*err == 0);
    int result;
    struct file *f;
    struct process *curprocess = get_curprocess();

    // get file
    f = ft_getfile(curprocess->file_table, fd, err);
    if (f == NULL) {
        assert(*err);
        return -1;
    }

    // remove file from filetable
    // filetable is responsible for properly closing the file
    result = ft_removefile(curprocess->file_table, fd);
    if (result) {
        *err = result;
        return -1;
    }

    return 0;
}
