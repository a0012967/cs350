#include <syscall.h>
#include <vfs.h>
#include <process.h>
#include <curprocess.h>
#include <filetable.h>
#include <kern/errno.h>
#include <lib.h>

int sys_close(int fd, int *err) {
    assert(*err == 0);
    int result;
    struct file *f;

    // get file
    f = ft_getfile(curprocess->file_table, fd, err);
    if (f == NULL) {
        assert(*err);
        return -1;
    }

    // close file
    vfs_close(f->v);

    // remove file from per-process filetable
    result = ft_removefile(curprocess->file_table, fd);
    if (result) {
        *err = result;
        return -1;
    }



    return 0;
}
