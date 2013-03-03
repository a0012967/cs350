#include <syscall.h>
#include <vfs.h>

int sys_close(int fd) {
    (void) fd;

    struct vnode *v; // get it from table
    vfs_close(v);

    return 0;
}
