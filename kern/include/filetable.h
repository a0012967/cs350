#ifndef _FILETABLE_H_
#define _FILETABLE_H_

#include <types.h>
#include <uio.h>

/* FILE STUFF */
struct file {
    int status;

    struct uio u;
    struct vnode *v;
    struct lock *file_lock; // when doing operations on the lock
};


/* CONSOLE VNODE */
struct vnode *vn_console;


// creates an file. returns NULL on fail
struct file* f_create(struct uio u, struct vnode *v);
// free memory used by file
void f_destroy(struct file *f);



/* FILE TABLE STUFF */
struct filetable;
// creates file table. returns NULL on failure
struct filetable* ft_create();
void ft_destroy(struct filetable *ft);
// returns index of file in table on success
// returns -1 if there was an error. changes value of error
int ft_storefile(struct filetable *ft, struct file* f, int *err);
// returns 0 if successful. returns error code otherwise
int ft_removefile(struct filetable *ft, int fd);
// SUCCESS: returns file stored at given fd
// FAILURE: returns NULL and updates value of err
struct file* ft_getfile(struct filetable *ft, int fd, int *err);




/* Opening console devices */
void console_files_bootstrap();


// USE THE BELOW only for debugging purposes!
// returns number of files in filetable
// DON'T USE THIS TO ITERATE THROUGH THE TABLE. IT HAS HOLES INSIDE
int ft_numfiles(struct filetable *ft);
// returns number of entries (including NULL) inside the filetable
int ft_getsize(struct filetable *ft);

#endif /* _FILETABLE_H_ */
