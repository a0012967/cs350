#include <filetable.h>
#include <table.h>
#include <synch.h>
#include <kern/errno.h>
#include <vnode.h>
#include <lib.h>


/*************************
 * FILE STUFF
 *************************/

struct file* f_create(struct uio u, struct vnode *v) {
    struct file *f = kmalloc(sizeof(struct file));
    if (f == NULL) {
        DEBUG(DB_EXEC, "FILE: failed creating file\n");
        return NULL;
    }

    f->u = u;
    f->v = v;

    return f;
}

void f_destroy(struct file *f) {
    assert(f != NULL);
    kfree(f);
}



/*************************
 * FILE TABLE STUFF
 *************************/

struct filetable {
    struct table *files;
    struct lock *ft_lock;
};

// creates file table. returns NULL on failure
struct filetable* ft_create() {
    struct filetable *ft = kmalloc(sizeof(struct filetable));
    if (ft == NULL) {
        DEBUG(DB_EXEC, "FILETABLE: failed creating filetable\n");
        return NULL;
    }

    ft->files = tab_create();
    if (ft->files == NULL) {
        DEBUG(DB_EXEC, "FILETABLE: failed creating filetable array\n");
        kfree(ft);
        return NULL;
    }

    ft->ft_lock = lock_create("ft_lock");
    if (ft->ft_lock == NULL) {
        DEBUG(DB_EXEC, "FILETABLE: failed creating filetable lock\n");
        tab_destroy(ft->files);
        kfree(ft);
        return NULL;
    }

    return ft;
}

// destroys file table
void ft_destroy(struct filetable *ft) {
    assert(ft != NULL);
    // destroy all elements in table
    int i=0;
    for (i=0; i < tab_getsize(ft->files); i++) {
        struct file *f = tab_getguy(ft->files, i);
        // remember files could be null so be careful
        if (f != NULL)
            f_destroy((struct file*)f);
    }
    tab_destroy(ft->files);
    lock_destroy(ft->ft_lock);
    kfree(ft);
}

// returns index of file in table on success
// returns -1 if there was an error. changes value of error
int ft_storefile(struct filetable *ft, struct file* f, int *err) {
    assert(ft != NULL && f != NULL && err != NULL);
    assert(*err == 0);

    int result;

    lock_acquire(ft->ft_lock);
        result = tab_add(ft->files, f, err);
        if (result == -1) {
            goto fail;
        }
        assert(result >= 0);
        assert(*err == 0);
    lock_release(ft->ft_lock);
    return result;

fail:
    lock_release(ft->ft_lock);
    assert(*err);
    assert(result == -1);
    return -1;
}

// returns 0 if successful
// frees memory used by the file
int ft_removefile(struct filetable *ft, int fd) {
    assert(ft != NULL);
    int result;

    lock_acquire(ft->ft_lock);
        if (fd < 0 || fd >= tab_getsize(ft->files)) {
            result = ENOENT;
            goto fail;
        } else {
            // get file to be removed
            void *file = tab_getguy(ft->files, fd);

            // remove reference of file from array
            result = tab_remove(ft->files, fd);

            // TODO: change how to handle failure on remove
            assert(result == 0);

            // free memory used by file
            f_destroy((struct file *)file);

            // success
            result = 0;
        }
    lock_release(ft->ft_lock);
    return result;

fail:
    lock_release(ft->ft_lock);
    return result;
}

// SUCCESS: returns file stored at given fd
// FAILURE: returns NULL and updates value of err
struct file* ft_getfile(struct filetable *ft, int fd, int *err) {
    assert(ft != NULL);
    int size;
    struct file *ret;

    lock_acquire(ft->ft_lock);
        size = tab_getnum(ft->files);

        // out of bounds
        if (fd < 0 && fd >= size) {
            *err = ENOENT; // no such file
            goto fail;
        }

        ret = (struct file*)tab_getguy(ft->files, fd);

        // file has been removed
        if (ret == NULL) {
            *err = ENOENT;
            goto fail;
        }
    lock_release(ft->ft_lock);

    return ret;

fail:
    lock_release(ft->ft_lock);
    assert(*err);
    return NULL;
}

