#include <syscall.h>
#include <file.h>
#include <filetable.h>
#include <uio.h>
#include <types.h>
#include <lib.h>
#include <process.h>
#include <processtable.h>
#include <thread.h>
#include <curthread.h>
#include <vnode.h>
#include <synch.h>
#include <kern/errno.h>
#include <vfs.h>
#include <kern/unistd.h>
#include <machine/spl.h>
#include <kern/errno.h>
#include <vm.h>

// TODO ENOSPC	There is no free space remaining on the filesystem containing the file.
// TODO EIO	A hardware I/O error occurred writing the data.
int sys_write(int fd, void *buf, size_t buflen, int *err) {
    assert(err != NULL);
    assert(*err == 0);

    int result;
    int file_status;
    struct uio u;
    struct file* fi;

    struct process *curprocess = processtable_get(curthread->pid);

    // get file at the specified handle
    fi = ft_getfile(curprocess->file_table, fd, err);
    if (fi == NULL) {
        *err = EBADF; // invalid fd
        return -1;
    }

    lock_acquire(fi->file_lock);

        // check if file is open for writing
        file_status = fi->status & O_ACCMODE;
        if (file_status != O_WRONLY && file_status != O_RDWR) {
            *err = EBADF;
            goto fail;
        }

        // check for valid buffer
        if (buf == NULL) {
            *err = EFAULT;
            goto fail;
        }

        // check if the buffer is in appropriate addr
		int dest;
		int ptr_check = copyin((userptr_t)buf, &dest, sizeof(buf));
		if (ptr_check !=0) {
			*err = ptr_check;
			goto fail;
		}

        // set up uio for writing
        u.uio_iovec.iov_ubase = buf;
        u.uio_iovec.iov_len = buflen;
        u.uio_rw = UIO_WRITE;
        u.uio_resid = buflen;
        u.uio_space = curthread->t_vmspace;
        u.uio_segflg = UIO_USERSPACE;
        u.uio_offset = fi->offset;

        // write to file
        result = VOP_WRITE(fi->v, &u);
        if (*err !=0) {
            goto fail;
        }

        fi->offset = u.uio_offset;

    lock_release(fi->file_lock);

    return buflen - u.uio_resid;

fail:
    assert(*err);
    lock_release(fi->file_lock);
    return -1;

}
