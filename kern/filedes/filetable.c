#include <filetable.h>
#include <table.h>
#include <synch.h>
#include <kern/errno.h>
#include <vnode.h>
#include <lib.h>

#include <curprocess.h>	
#include <process.h>
#include <vfs.h>
#include <kern/unistd.h>

/*************************
 * FILE STUFF
 *************************/

struct file* f_create(int status, int offset, struct vnode *v) {
    struct file *f = kmalloc(sizeof(struct file));
    if (f == NULL) {
        DEBUG(DB_EXEC, "FILE: failed creating file\n");
        return NULL;
    }

    f->file_lock = lock_create("file_lock");
    if (f == NULL) {
        DEBUG(DB_EXEC, "FILE: failed creating file lock\n");
        kfree(f);
        return NULL;
    }

    f->status = status;
    f->offset = offset;
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
    assert(ft != NULL && f != NULL);
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
        size = tab_getsize(ft->files);

        // out of bounds
        if (fd < 0 || fd >= size) {
            *err = EBADF; // no such file
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


/* CONSOLE DEVICES FILES 
 * Opens the console and creates a file for stdin, stdout, & stderr
 * Files stored in file_table at index 0, 1, and 2 respectively
 */
void console_files_bootstrap() {
    // open the console
    int err, result;
    struct vnode *vn;
    struct uio u_in, u_out, u_err;    
    struct file *stdinfile, *stdoutfile, *stderrfile;
    
    char *console = NULL;
    console = kstrdup("con:");
    
    // open the console
    err = vfs_open(console, O_RDONLY, &vn);
    if(err){
        panic("console files: Could not open console\n");
    }
    
    // create and add stdin file to file_table[0]
    u_in.uio_space = curprocess->p_thread->t_vmspace;
    u_in.uio_offset = 0;
    u_in.uio_rw = UIO_READ;
    
    stdinfile = f_create( u_in, vn);
    if (stdinfile == NULL) {
        panic("Could not create an open file entry for stdin\n");
    }
    
    stdinfile->status = O_RDONLY;
    
    // store stdinfile at file_table[0]
    result  = ft_storefile(curprocess->file_table, stdinfile, &err);
    if (result == -1) {
        panic("Could not add stdin file to filetable");
    }

    // create and add stdout file to file_table[1]
    u_out.uio_space = curprocess->p_thread->t_vmspace;
    u_out.uio_offset = 0;
    u_out.uio_rw = UIO_WRITE;

    stdoutfile = f_create( u_out, vn);
    if (stdoutfile == NULL) {
        panic("Could not create an open file entry for stdout\n");
    }

    stdoutfile->status = O_WRONLY;

    result  = ft_storefile(curprocess->file_table, stdoutfile, &err);
    if (result == -1) {
        panic("Could not add stdout file to filetable");
    }
        
    // create and add stderr file to file_table
    u_err.uio_space = curprocess->p_thread->t_vmspace;
    u_err.uio_offset = 0;
    u_err.uio_rw = UIO_WRITE;

    stderrfile = f_create( u_err, vn);
    if (stderrfile == NULL) {
        panic("Could not create an open file entry for stderr\n");
    }

    stderrfile->status = O_WRONLY;

    // store stderrfile at file_table[2]
    result  = ft_storefile(curprocess->file_table, stderrfile, &err);
    if (result == -1) {
        panic("Could not add stdin file to filetable");
    }
    kfree(console);
    
    struct file *fi;
    int err2;
    fi = ft_getfile(curprocess->file_table, 0, &err2);
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
