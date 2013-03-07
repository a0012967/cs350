#ifndef _OPENFILETABLE_H_
#define _OPENFILETABLE_H_

// TODO: rethink number of max files open
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

/* OPEN FILE STUFF */
// creates an open file. returns NULL on fail
struct openfile* of_create(int status, struct uio *u, struct vnode *v);
// free memory used by openfile
void of_destroy(struct openfile *of);


/* OPEN FILE TABLE STUFF */

// initialize openfiletable
// should be called on boot
void openfile_table_bootstrap();

// returns index of openfile in table on success
// returns -1 if there was an error. changes value of error
int oft_storefile(struct openfile* file, int *error);
// removes open file from open file table and destroys it
int oft_removefile(int fd);
struct openfile* oft_getfile(int fd);

#endif /* _OPENFILETABLE_H_ */
