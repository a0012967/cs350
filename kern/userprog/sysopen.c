#include <types.h>
#include <lib.h>
#include <uio.h>
#include <elf.h>
#include <curprocess.h>
#include <process.h>
#include <thread.h>
#include <vnode.h>
#include <syscall.h>
#include <kern/errno.h>
#include <vfs.h>
#include <filetable.h>

// returns -1 if error occured and changes content of err
int sys_open(const char *filename, int flags, int *err) {
    assert(filename != NULL);
    assert(*err == 0);

    struct vnode *v;
    struct uio u;
    int ret;

    ret = vfs_open((char*)filename, flags, &v);
    if (ret) {
        *err = ret;
        return -1;
    }

    // start storing info in OPEN FILE STUFF
    u.uio_offset = 0;
    // TODO: not sure about this
    u.uio_space = curprocess->p_thread->t_vmspace;

    struct file *f = f_create(u, v);
    assert(f != NULL);
    f->status = flags;
    ret = ft_storefile(curprocess->file_table, f, err);

    return ret;
}
