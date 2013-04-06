#include <types.h>
#include <lib.h>
#include <kern/limits.h>
#include <uio.h>
#include <elf.h>
#include <curthread.h>
#include <process.h>
#include <processtable.h>
#include <thread.h>
#include <vnode.h>
#include <syscall.h>
#include <kern/errno.h>
#include <vfs.h>
#include <file.h>
#include <filetable.h>

// returns 1 if INVALID, 0 otherwise
int filename_invalid(const char *filename, int *err) {
    if (filename == NULL) {
        *err = EFAULT;
        return 1;
    }

	// check if the buffer is in appropriate addr
    char foo[PATH_MAX];
	*err = copyinstr((userptr_t)filename, foo, sizeof(foo), NULL);
	if (*err) {
		return 1;
	}

    return 0;
}

// returns -1 if error occured and changes content of err
int sys_open(const char *filename, int flags, int *err) {
    struct process *curprocess = get_curprocess();
    struct vnode *v;
    int ret;

    assert(err != NULL);
    assert(*err == 0);

    if (filename_invalid(filename, err)) {
        assert(*err);
        return -1;
    }

    ret = vfs_open((char*)filename, flags, &v);
    if (ret) {
        *err = ret;
        return -1;
    }

    // create a file
    struct file *f = f_create(flags, 0, v);
    if (f == NULL) {
        vfs_close(v);
        *err = ENOMEM;
        return -1;
    }

    // add the file to ft_storefile
    ret = ft_storefile(curprocess->file_table, f, err);
    if (ret == -1) {
        f_destroy(f);
        vfs_close(v);
        return -1;
    }

    return ret;
}
