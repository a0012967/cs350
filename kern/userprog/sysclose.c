#include <syscall.h>
#include <vfs.h>
#include <file.h>
#include <filetable.h>
#include <curthread.h>
#include <thread.h>
#include <process.h>
#include <processtable.h>
#include <systemfiletable.h>
#include <vnode.h>
#include <lib.h>
#include <types.h>
#include <kern/errno.h>

int sys_close(int fd, int *err) {
    assert(err != NULL);
    assert(*err == 0);
    int result;
    struct file *f;
    struct process *curprocess = processtable_get(curthread->pid);

    // get file
    f = ft_getfile(curprocess->file_table, fd, err);
    if (f == NULL) {
        assert(*err);
        return -1;
    }

    // close file
    if (f->count > 1)
        VOP_DECOPEN(f->v);
    else
        vfs_close(f->v);

    // remove file from per-process filetable
    result = ft_removefile(curprocess->file_table, fd);
    if (result) {
        *err = result;
        return -1;
    }

    // calls remove on the file for the systemwide filetable
    result = systemft_remove(f);
    assert(result == 0);

    return 0;
}
