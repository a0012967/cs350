#include <file.h>
#include <types.h>
#include <lib.h>
#include <synch.h>

struct file* f_create(int status, int offset, struct vnode *v) {
    struct file *f = kmalloc(sizeof(struct file));
    if (f == NULL) {
        return NULL;
    }

    f->file_lock = lock_create("file_lock");
    if (f == NULL) {
        kfree(f);
        return NULL;
    }

    f->status = status;
    f->offset = offset;
    f->v = v;

    // system-wide-filetable-related internals
    f->count = 0;
    f->prev = NULL;
    f->next = NULL;

    return f;
}

void f_destroy(struct file *f) {
    assert(f != NULL);
    kfree(f);
}
