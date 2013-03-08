#include <syscall.h>
#include <vfs.h>
#include <process.h>
#include <curprocess.h>
#include <filetable.h>
#include <kern/errno.h>

int sys_close(int fd, int *err) {
    struct file *f = ft_getfile(curprocess->file_table, fd, err);
    if (err) {
        return -1;
    }
    vfs_close(f->v);
    return 0;
}
