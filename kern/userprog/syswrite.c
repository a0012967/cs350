#include <syscall.h>
#include <curprocess.h>
#include <filetable.h>
#include <vfs.h>
#include <uio.h>
#include <types.h>
#include <lib.h>
#include <process.h>
#include <thread.h>
#include <vnode.h>
#include <kern/errno.h>
#include <vfs.h>
#include <filetable.h>
#include <kern/unistd.h>

// TODO ENOSPC	There is no free space remaining on the filesystem containing the file.
// TODO EIO	A hardware I/O error occurred writing the data.

#if OPT_A2

	int sys_write(int fd, const void *buf, size_t buflen, int *err) {
		
		int result;
		int file_status;
        struct file* fi;
		
		// get file at the specified handle
		fi = ft_getfile(curprocess->file_table, fd, err);
		if (fi == NULL) {
	        *err = EBADF; // invalid fd
	        return -1;
        }
		
        lock_acquire(fi->file_lock);
		    
		    // check if file is open for writing
	        file_status = fi->status & O_ACCMODE;
            switch (file_status) {
            case O_WRONLY:
            case O_RDWR:
            break;
            default:
                *err = EBADF;
                lock_release(fi->file_lock);
            return -1;
            }
	        
	        // check for valid buffer
	        if (buf == NULL) {
                *err = EFAULT;
                lock_release(fi->file_lock);
                return -1;
	        }
	        
	        // check for valid buffer region
            result = buffer_check(buf, buflen);
            if (result == -1) {
                *err = EFAULT;
                lock_release(fi->file_lock);          
                return -1;
            }

            // set up uio for writing		    
		    struct uio u;
	        u.uio_iovec.iov_ubase = buf;
	        u.uio_iovec.iov_len = buflen;
	        u.uio_rw = UIO_WRITE;
	        u.uio_resid = buflen;
	        u.uio_space = curprocess->p_thread->t_vmspace;
		    u.uio_segflg = UIO_USERSPACE;
		    u.uio_offset = fi->u.uio_offset;
		    
		    // write to file
		    *err = VOP_WRITE(fi->v, &u);
            if (*err !=0) {
                lock_release(fi->file_lock);
                return -1;
            }
		    
		    fi->u.uio_offset = u.uio_offset;
		    //*err = buflen - u.uio_resid;
		    
		    assert(*err >= 0);
		lock_release(fi->file_lock);
		
	    return buflen - u.uio_resid;
	}
	
	
/* Memory region check function. Checks to make sure the block
 * of user memory provided (buf to buflen) falls within the proper
 * userspace region. If it does not, -1 is returned. 
 * Returns 0 if valid buffer
 * Note: most of this code is taken from copycheck() in copyinout.c
 */
/*int buffer_check(void *buf, size_t buflen){
	vaddr_t bot, top;
	bot = (vaddr_t) buf;
	top = bot + buflen - 1;
	if (buf == NULL) {
		return -1;
	}
	if (top < bot) {
		//addresses wrapped around
		return -1;
	}
	if (bot >= USERTOP){
		//region is within the kernel
		return -1;
	}
	if (top >= USERTOP) {
		//region overlaps the kernel
		return -1;
	}
	return 0;
}*/
	
	
#endif /* OPT_A2 */
