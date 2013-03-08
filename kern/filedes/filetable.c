#include <filetable.h>
#include <array.h>
#include <synch.h>
#include <kern/errno.h>
#include <vnode.h>
#include <lib.h>


/*************************
 * FILE STUFF
 *************************/

struct file* f_create() {
    struct file *f = kmalloc(sizeof(struct file));
    if (f == NULL) {
        DEBUG(DB_EXEC, "FILE: failed creating file\n");
        return NULL;
    }
    return f;
}

void f_destroy(struct file *f) {
    kfree(f);
}



/*************************
 * FILE TABLE STUFF
 *************************/

struct filetable {
    struct array *files;
    struct lock *ft_lock;
};

// creates file table. returns NULL on failure
struct filetable* ft_create() {
    struct filetable *ft = kmalloc(sizeof(struct filetable));
    if (ft == NULL) {
        DEBUG(DB_EXEC, "FILETABLE: failed creating filetable\n");
        return NULL;
    }
    
    ft->files = array_create();
    if (ft->files == NULL) {
        DEBUG(DB_EXEC, "FILETABLE: failed creating filetable array\n");
        kfree(ft);
        return NULL;
    }

    ft->ft_lock = lock_create("ft_lock");
    if (ft->ft_lock == NULL) {
        DEBUG(DB_EXEC, "FILETABLE: failed creating filetable lock\n");
        array_destroy(ft->files);
        kfree(ft);
        return NULL;
    }

    return ft;
}

// destroys file table
void ft_destroy(struct filetable *ft) {
    assert(ft != NULL);
    // destroy all elements in array
    int i=0;
    for (i=0; i < array_getnum(ft->files); i++) {
        struct file *f = array_getguy(ft->files, i);
        f_destroy((struct file*)f);
    }
    array_destroy(ft->files);
    lock_destroy(ft->ft_lock);
    kfree(ft);
}

// returns index of file in table on success
// returns -1 if there was an error. changes value of error
int ft_storefile(struct filetable *ft, struct file* f, int *err) {
    assert(ft != NULL && f != NULL && err != NULL);
    int result;

    lock_acquire(ft->ft_lock);
        result = array_add(ft->files, f);
        if (result) {
            *err = result;
            result = -1;
        }
        else {
            result = array_getnum(ft->files) - 1; // last index
            // TODO: remove later
            assert(result >= 0);
        }
    lock_release(ft->ft_lock);

    return result;
}

// returns 0 if successful
int ft_removefile(struct filetable *ft, int fd) {
    assert(ft != NULL);
    int result;

    lock_acquire(ft->ft_lock);
        if (fd < 0 || fd >= array_getnum(ft->files))
            result = ENOENT;
        else {
            // get file to be removed
            void *file = array_getguy(ft->files, fd);

            // removed reference of file from array
            array_remove(ft->files, fd);

            // free memory used by file
            f_destroy((struct file *)file);

            // success
            result = 0;
        }
    lock_release(ft->ft_lock);

    return result;
}

// returns null if invalid index for now
struct file* ft_getfile(struct filetable *ft, int fd) {
    assert(ft != NULL);
    int size;
    struct file *ret;

    lock_acquire(ft->ft_lock);
        size = array_getnum(ft->files);
        if (fd < 0 || fd >= size) {
            ret = NULL;
        } else {
            ret = (struct file*)array_getguy(ft->files, fd);
        }
    lock_release(ft->ft_lock);

    return ret;
}

