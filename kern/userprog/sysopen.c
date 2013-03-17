#include <types.h>
#include <lib.h>
#include <kern/limits.h>
#include <uio.h>
#include <elf.h>
#include <curthread.h>
#include <process.h>
#include <processtable.h>
#include <thread.h>
#include <vnode.h>
#include <syscall.h>
#include <kern/errno.h>
#include <vfs.h>
#include <file.h>
#include <filetable.h>
#include <systemfiletable.h>

// returns -1 if error occured and changes content of err
int sys_open(const char *filename, int flags, int *err) {
    struct process *curprocess = processtable_get(curthread->pid);
    struct vnode *v;
    int ret;

    assert(err != NULL);
    assert(*err == 0);

    if (filename == NULL) {
        *err = EFAULT;
        return -1;
    }

    // check for validity of memory address
    if (buffer_check((void*)filename, PATH_MAX)) {
        *err = EFAULT;
        return -1;
    }

    ret = vfs_open((char*)filename, flags, &v);
    if (ret) {
        *err = ret;
        return -1;
    }

    // create a file
    struct file *f = f_create(flags, 0, v);
    if (f == NULL) {
        vfs_close(v);
        *err = ENOMEM;
        return -1;
    }

    // add the file to systemwide filetable
    ret = systemft_insert(f);
    if (ret) {
        vfs_close(v);
        *err = ret;
        return -1;
    }

    // add the file to ft_storefile as well
    ret = ft_storefile(curprocess->file_table, f, err);
    if (ret == -1) {
        // remove from system file table
        // systemfiletable will take care of destroying the file
        systemft_remove(f);
        vfs_close(v);
        return -1;
    }

    return ret;
}
