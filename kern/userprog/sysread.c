#include <syscall.h>
#include <lib.h>
#include <types.h>
#include <curprocess.h>
#include <thread.h>
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
#include "opt-A2.h"

    int sys_read(int fd, void *buf, size_t buflen,  int *retval) {


    int result;
    int how;
    struct file *file;


    file = ft_getfile(curprocess->file_table, fd, retval);	
    lock_acquire(file->file_lock);
    if (file == NULL) {
        *retval = EBADF; // invalid fd
        lock_release(file->file_lock);
        return -1;

    }
    // check that the file is opened for reading
    how = file->status & O_ACCMODE;
    switch (how) {
        case O_RDONLY:
        case O_RDWR:
            break;
        default:
            *retval = EBADF;
            lock_release(file->file_lock);
            return -1;
    }

    //check for valid buffer
    if (buf == NULL) {
        *retval = EFAULT;
        lock_release(file->file_lock);
        return -1;
    }

    // check for valid buffer region
    result = buffer_check(buf, buflen);
    if (result == -1) {
        *retval = EFAULT;
        lock_release(file->file_lock);
        return -1;

    }

    // initialize the file's uio for reading
    result = uio_init(file, buf, buflen);
    if (result == -1 ) {
        lock_release(file->file_lock);
        //TODO: figure out if need to set retval
        return -1;
    }

    // errors:
    // invalid fd
    // buflen > file
    // think about end of file
    *retval = VOP_READ(file->v, &(file->u));
    if (*retval !=0) {
        lock_release(file->file_lock);
        return -1;
    }
    // return the number of bytes read
    *retval = buflen - file->u.uio_resid;
    assert(*retval >= 0);
    lock_release(file->file_lock);
    return 0;
}

//  initializes the uio of the file for reading
//  returns 0 on success
//  returns -1 if error occured and changes err to errno
int uio_init(struct file *file, void * buf, size_t buflen){

    assert(file != NULL);

    file->u.uio_iovec.iov_un.un_ubase = buf;
    file->u.uio_iovec.iov_len = buflen;
    file->u.uio_rw = UIO_READ;
    file->u.uio_resid = buflen;
    file->u.uio_space = curprocess->p_thread->t_vmspace;

    return 0;
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
