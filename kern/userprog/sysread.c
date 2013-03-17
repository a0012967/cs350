#include <syscall.h>
#include <lib.h>
#include <types.h>
#include <curthread.h>
#include <addrspace.h>
#include <thread.h>
#include <process.h>
#include <processtable.h>
#include <vfs.h>
#include <machine/spl.h>
#include <kern/errno.h>
#include <file.h>
#include <filetable.h>
#include <uio.h>
#include <lib.h>
#include <vm.h>
#include <kern/unistd.h>
#include <vnode.h>
#include <synch.h>

int sys_read(int fd, userptr_t buf, size_t buflen,  int *err) {
    struct process *curprocess = processtable_get(curthread->pid);
    int result;
    int how;
    struct file *file;
    struct uio u;

    assert(err != NULL); // err should exist
    assert(*err == 0); // error should be cleared when calling this

	//check for valid buffer
	if (buf == NULL) {
	    *err = EFAULT;
		return -1;
	}

	// check if the buffer is in appropriate addr
	int dest;
	int ptr_check = copyin((userptr_t)buf, &dest, sizeof(buf));
	if (ptr_check !=0) {
		*err = ptr_check;
		return -1;
	}

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
