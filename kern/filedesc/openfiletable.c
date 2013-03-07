#include <openfiletable.h>
#include <types.h>
#include <array.h>
#include <synch.h>
#include <kern/errno.h>
#include <vnode.h>
#include <lib.h>
#include <uio.h>


// TODO: put openfiletable.c into folder by itself
static struct array *oft;
static struct lock *oft_lock;


/*************************
 * OPEN FILE STUFF
 *************************/

struct openfile* of_create(int status, struct uio *u, struct vnode *v) {
    struct openfile *of = kmalloc(sizeof(struct openfile));
    if (of != NULL) {
        of->status = status;
        // keep references
        of->u = u; // TODO: might change how to store u
        of->v = v;
    }
    return of;
}

void of_destroy(struct openfile *of) {
    // TODO: might change how to store u
    kfree(of);
}



/*************************
 * OPEN FILE TABLE STUFF
 *************************/

// bootstrap filetable
void openfile_table_bootstrap() {
    // instantiate array of oft
    oft = array_create();
    if (oft == NULL) {
        panic("filetable: Could not create open file table array\n");
    }

    // instantiate lock
    oft_lock = lock_create("oft");
    if (oft_lock == NULL) {
        panic("filetable: Could not create open file table lock\n");
    }
}

// returns index of openfile in table on success
// returns -1 if there was an error. changes value of error
int oft_storefile(struct openfile* of, int *error) {
    int result;

    lock_acquire(oft_lock);
        if (array_getnum(oft) >= MAXFILES) {
            // too many open oft in system
            *error = ENFILE;
            result = -1; // signal for error
        } else {
            result = array_add(oft, of);
            // update result as index where of was inserted
            if (result == 0) {
                result = array_getnum(oft)-1;
            }
        }
    lock_release(oft_lock);

    return result;
}

// returns 0 if successful
int oft_removefile(int fd) {
    int result;

    lock_acquire(oft_lock);
        if (fd < 0 || fd >= array_getnum(oft))
            result = ENOENT;
        else {
            // get openfile to be removed
            void *file = array_getguy(oft, fd);

            // removed reference of openfile from array
            array_remove(oft, fd);

            // free memory used by openfile
            of_destroy((struct openfile *)file);

            // success
            result = 0;
        }
    lock_release(oft_lock);

    return result;
}

// returns null if invalid index for now
struct openfile* oft_getfile(int fd) {
    int size;
    struct openfile *ret;

    lock_acquire(oft_lock);
        size = array_getnum(oft);
        if (fd < 0 || fd >= size) {
            ret = NULL;
        } else {
            ret = (struct openfile *)array_getguy(oft, fd);
        }
    lock_release(oft_lock);

    return ret;
}

