#include <syscall.h>
#include <lib.h>
#include <types.h>
#include <curprocess.h>
#include <thread.h>
#include <curthread.h>
#include <process.h>
#include <vfs.h>
#include <machine/spl.h>
#include <kern/errno.h>
#include <filetable.h>
#include <uio.h>
#include <lib.h>
#include <vm.h>
#include <kern/unistd.h>
#include <vnode.h>
#include <synch.h>

int sys_read(int fd, userptr_t buf, size_t buflen,  int *err) {
    int result;
    int how;
    struct file *file;
    struct uio u;

    assert(err != NULL); // err should exist
    assert(*err == 0); // error should be cleared when calling this


    file = ft_getfile(curprocess->file_table, fd, err);

    if (file == NULL) {
        // err should have been updated with the error code
        assert(*err != 0);

        if (*err == ENOENT){ // file closed or doesnt exist
            *err =  EBADF;
        }
        return -1;
    }

    // start the lock
    lock_acquire(file->file_lock);


    // check that the file is opened for reading
    how = file->status & O_ACCMODE;
    switch (how) {
        case O_RDONLY:
        case O_RDWR:
            break;
        default:
            *err = EBADF;
            goto fail;
    }

    //check for valid buffer
    if (buf == NULL) {
        *err = EFAULT;
        goto fail;
    }

    // check for valid buffer region
    result = buffer_check(buf, buflen);
    if (result == -1) {
        *err = EFAULT;
        goto fail;
    }

    // initialize the uio for reading
    u.uio_iovec.iov_ubase = buf;
    u.uio_iovec.iov_len = buflen;
    u.uio_offset = file->offset;
    u.uio_rw = UIO_READ;
    u.uio_resid = buflen;
    u.uio_segflg = UIO_USERSPACE;
    u.uio_space = curthread->t_vmspace;

    result = VOP_READ(file->v, &u);
    if (result !=0) {
        goto fail;
    }

    // return the number of bytes read
    result  = buflen - u.uio_resid;
    assert(result >= 0);
    // update the offset of the file
    file->offset = u.uio_offset;
    lock_release(file->file_lock);
    return result;

fail:
    assert(*err != 0);
    lock_release(file->file_lock);
    return -1;
}

/* Memory region check function. Checks to make sure the block
 * of user memory provided (buf to buflen) falls within the proper
 * userspace region. If it does not, -1 is returned. 
 * Returns 0 if valid buffer
 * Note: most of this code is taken from copycheck() in copyinout.c
 */
int buffer_check(void *buf, size_t buflen){
    vaddr_t bot, top;
    bot = (vaddr_t) buf;
    top = bot + buflen - 1;
    if (buf == NULL) {
        return -1;
    }
    if (top < bot) {
        /* addresses wrapped around */
        return -1;
    }
    if (bot >= USERTOP){
        /* region is within the kernel */
        return -1;
    }
    if (top >= USERTOP) {
        /* region overlaps the kernel */
        return -1;
    }
    return 0;
}
