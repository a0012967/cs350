#include <syscall.h>
#include <vfs.h>

int sys_open(const char *filename, int flags) {

    struct vnode *v;
    int result;

    // must create a file descriptor struct

    // cast to suppress warning
    result = vfs_open((char*)filename, flags, &v);
    if (result) {
        return result;
    }

    return 0;
}
