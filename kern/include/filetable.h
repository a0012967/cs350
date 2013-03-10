#ifndef _FILETABLE_H_
#define _FILETABLE_H_

#include <types.h>
#include <uio.h>

/* FILE STUFF */
struct file {
    struct int status;
    struct uio u;
    struct vnode *v;
};

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

#endif /* _FILETABLE_H_ */
