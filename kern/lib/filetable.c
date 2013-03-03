#include <filetable.h>
#include <types.h>
#include <array.h>
#include <synch.h>
#include <kern/errno.h>
#include <vnode.h>
#include <lib.h>

static struct array *files;
static struct lock *files_lock;

// bootstrap filetable
void ft_bootstrap() {
    files = array_create();
    if (files == NULL) {
        panic("filetable: Could not create files array\n");
    }

    files_lock = lock_create("files");
    if (files_lock == NULL) {
        panic("filetable: Could not create files lock\n");
    }
}

int storefile(struct vnode* file) {
    int result;

    lock_acquire(files_lock);
        if (array_getnum(files) == MAXFILES)
            result = ENFILE;
        else
            result = array_add(files, file);
    lock_release(files_lock);

    return result;
}

int removefile(int fd) {
    int result = 0;

    lock_acquire(files_lock);
        if (fd < 0 || fd >= array_getnum(files))
            result = ENOENT;
        else
            array_remove(files, fd);
    lock_release(files_lock);

    return result;
}

// returns null if invalid index for now
struct vnode* getfile(int fd) {
    int size;
    struct vnode *ret;

    lock_acquire(files_lock);
        size = array_getnum(files);
        if (fd < 0 || fd >= size)
            ret = NULL;
        else
            ret = array_getguy(files, fd);
    lock_release(files_lock);

    return ret;
}

