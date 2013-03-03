#include <openfiletable.h>
#include <types.h>
#include <array.h>
#include <synch.h>
#include <kern/errno.h>
#include <vnode.h>
#include <lib.h>
#include <uio.h>


static struct array *files;
static struct lock *files_lock;

struct openfile* of_create(int status, struct uio *u, struct vnode *v) {
    struct openfile *of = kmalloc(sizeof(struct openfile));
    if (of != NULL) {
        of->status = status;

        // keep references
        of->u = u;
        of->v = v;
    }
    return of;
}

void of_destroy(struct openfile *of) {
    kfree(of);
}

// bootstrap filetable
void ft_bootstrap() {
    // instantiate array of files
    files = array_create();
    if (files == NULL) {
        panic("filetable: Could not create files array\n");
    }

    // instantiate lock
    files_lock = lock_create("files");
    if (files_lock == NULL) {
        panic("filetable: Could not create files lock\n");
    }
}

// returns index of openfile in table on success
// returns -1 if there was an error. changes value of error
int storefile(struct openfile* of, int *error) {
    int result;

    lock_acquire(files_lock);
        if (array_getnum(files) >= MAXFILES) {
            // too many open files in system
            *error = ENFILE;
            result = -1; // signal for error
        } else {
            result = array_add(files, of);
        }
    lock_release(files_lock);

    return result;
}

// returns 0 if successful
int removefile(int fd) {
    int result;

    lock_acquire(files_lock);
        if (fd < 0 || fd >= array_getnum(files))
            result = ENOENT;
        else {
            // get openfile to be removed
            void *file = array_getguy(files, fd);

            // removed reference of openfile from array
            array_remove(files, fd);

            // free memory used by openfile
            of_destroy((struct openfile *)file);

            // success
            result = 0;
        }
    lock_release(files_lock);

    return result;
}

// returns null if invalid index for now
struct openfile* getfile(int fd) {
    int size;
    struct openfile *ret;

    lock_acquire(files_lock);
        size = array_getnum(files);
        if (fd < 0 || fd >= size) {
            ret = NULL;
        } else {
            ret = (struct openfile *)array_getguy(files, fd);
        }
    lock_release(files_lock);

    return ret;
}

