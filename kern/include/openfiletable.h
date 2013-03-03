#ifndef _OPENFILETABLE_H_
#define _OPENFILETABLE_H_

#define MAXFILES 1024


struct openfile {
    // TODO: not sure what to do with this yet
    int status;

    // TODO: how is uio cleaned up?
    // TODO: keep reference to u for now
    struct uio *u;

    // we just hold a pointer to the vnode
    struct vnode *v;
};

// creates an open file. returns NULL on fail
struct openfile* of_create(int status, struct uio *u, struct vnode *v);
// free memory used by openfile
void of_destroy(struct openfile *of);

// initialize openfiletable
// should be called on boot
void ft_bootstrap();

// returns index of openfile in table on success
// returns -1 if there was an error. changes value of error
int storefile(struct openfile* file, int *error);
int removefile(int fd);
struct openfile* getfile(int fd);

#endif /* _OPENFILETABLE_H_ */
