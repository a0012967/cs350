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
#include <openfiletable.h>

// returns -1 if error occured and changes content of err
int sys_open(const char *filename, int flags, int *err) {
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

    struct openfile *of = of_create(0, &u, v);
    ret = oft_storefile(of, err);

    return ret;
}
