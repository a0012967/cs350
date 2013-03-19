#include <file.h>
#include <filetable.h>
#include <table.h>
#include <synch.h>
#include <kern/errno.h>
#include <kern/limits.h>
#include <vnode.h>
#include <lib.h>
#include <process.h>
#include <processtable.h>
#include <curthread.h>
#include <vfs.h>
#include <kern/unistd.h>

struct filetable {
    struct table *files;
    struct lock *ft_lock;
};

// creates file table. returns NULL on failure
struct filetable* ft_create() {
    struct filetable *ft = kmalloc(sizeof(struct filetable));
    if (ft == NULL) {
        return NULL;
    }

    ft->files = tab_create();
    if (ft->files == NULL) {
        kfree(ft);
        return NULL;
    }

    ft->ft_lock = lock_create("ft_lock");
    if (ft->ft_lock == NULL) {
        tab_destroy(ft->files);
        kfree(ft);
        return NULL;
    }

    return ft;
}

// destroys file table
void ft_destroy(struct filetable *ft) {
    // destroy all elements in table
    int i;
    for (i=0; i < tab_getsize(ft->files); i++) {
        struct file *f = tab_getguy(ft->files, i);
        if (f != NULL) {
            tab_remove(ft->files, i);
            f->numrefs--;
            // close and destroy the file if there are no more references
            if (f->numrefs == 0) {
                vfs_close(f->v);
                f_destroy(f);
            }
        }
    }
    tab_destroy(ft->files);
    lock_destroy(ft->ft_lock);
    kfree(ft);
}

// returns index of file in table on success
// returns -1 if there was an error. changes value of error
int ft_storefile(struct filetable *ft, struct file* f, int *err) {
    assert(err != NULL);
    assert(*err == 0);
    int result;
    lock_acquire(ft->ft_lock);
        int numfiles = tab_getnum(ft->files);

        if (numfiles > PROC_NFILES_MAX)
            panic("FILETABLE: filetable implementation broken\n");

        if (numfiles == PROC_NFILES_MAX) {
            *err = EMFILE;
            goto fail;
        }

        result = tab_add(ft->files, f, err);

        if (result == -1) {
            goto fail;
        }

        assert(f->numrefs == 0);
        f->numrefs = 1; // increase number of references
    lock_release(ft->ft_lock);
    kprintf("ref: %d\n", f->v->vn_refcount);
    kprintf("open: %d\n", f->v->vn_refcount);
    kprintf("numrefs: %d\n", f->numrefs);
    return result;

fail:
    lock_release(ft->ft_lock);
    return -1;
}

// returns 0 if successful
// decrements numrefs
int ft_removefile(struct filetable *ft, int fd) {
    int result = 0;
    struct file *f;

    lock_acquire(ft->ft_lock);
        if (fd < 0 || fd >= tab_getsize(ft->files)) {
            result = ENOENT;
            goto fail;
        } else {
            f = tab_getguy(ft->files, fd);
            if (f != NULL) {
                result = tab_remove(ft->files, fd);
                assert(result == 0);
                f->numrefs--; // decrement number of references
                if (f->numrefs == 0) {
                    vfs_close(f->v);
                    f_destroy(f);
                }
            }
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
    int size;
    struct file *ret;

    lock_acquire(ft->ft_lock);
        size = tab_getsize(ft->files);

        // out of bounds
        if (fd < 0 || fd >= size) {
            *err = EBADF; // no such file
            goto fail;
        }

        ret = (struct file*)tab_getguy(ft->files, fd);

        // file has been removed
        if (ret == NULL) {
            *err = EBADF;
            goto fail;
        }
    lock_release(ft->ft_lock);

    return ret;

fail:
    lock_release(ft->ft_lock);
    return NULL;
}

int ft_duplicate(struct filetable *ft, struct filetable **new_ft) {
    assert(*new_ft != NULL);

    struct filetable *nft;
    int i, size, err = 0;

    lock_acquire(ft->ft_lock);
        nft = *new_ft;
        err = tab_duplicate(ft->files, &(nft->files));
        assert(!err); // TODO:

        size = tab_getsize(nft->files);

        /*
        // test validity
        assert(size == tab_getsize(ft->files));
        for (i=0; i<size; i++) {
            void *ptr1, *ptr2;
            ptr1 = tab_getguy(ft->files, i);
            ptr2 = tab_getguy(nft->files, i);
            assert(ptr1 == ptr2);
        }
        */

        for (i=0; i<size; i++) {
            struct file *f = (struct file*)tab_getguy(nft->files, i);
            if (f != NULL) {
                f->numrefs++; // increase references
            }
        }
    lock_release(ft->ft_lock);

    return err;
}

/* CONSOLE DEVICES FILES 
 * Opens the console and creates a file for stdin, stdout, & stderr
 * Files stored in file_table at index 0, 1, and 2 respectively
 */
void console_files_bootstrap() {
    int err;
    struct vnode *vn;
    struct file *f_stdin, *f_stdout, *f_stderr;
    struct process *curprocess = processtable_get(curthread->pid);

    char *console = NULL;
    console = kstrdup("con:");

    // open the console
    err = vfs_open(console, O_RDONLY, &vn); assert(!err);

    // create files
    f_stdin  = f_create(O_RDONLY, 0, vn); assert(f_stdin );
    f_stdout = f_create(O_WRONLY, 0, vn); assert(f_stdout);
    f_stderr = f_create(O_WRONLY, 0, vn); assert(f_stderr);

    // store files in the filetable
    ft_storefile(curprocess->file_table, f_stdin , &err); assert(!err);
    ft_storefile(curprocess->file_table, f_stdout, &err); assert(!err);
    ft_storefile(curprocess->file_table, f_stderr, &err); assert(!err);

    // TODO: might change this. looks ugly
    f_stdin->numrefs  = 3;
    f_stdout->numrefs = 3;
    f_stderr->numrefs = 3;

    //increase vnode ref count for stdout and stderr
    vnode_incref(vn);
    vnode_incref(vn);
    kfree(console);
}


// USE THE BELOW only for debugging purposes!
// returns number of files in filetable
// DON'T USE THIS TO ITERATE THROUGH THE TABLE. IT HAS HOLES INSIDE
int ft_numfiles(struct filetable *ft) {
    assert(ft != NULL);
    lock_acquire(ft->ft_lock);
        int ret = tab_getnum(ft->files);
    lock_release(ft->ft_lock);
    return ret;
}

// returns number of entries (including NULL) inside the filetable
int ft_getsize(struct filetable *ft) {
    assert(ft != NULL);
    lock_acquire(ft->ft_lock);
        int ret = tab_getsize(ft->files); 
    lock_release(ft->ft_lock);
    return ret;
}
