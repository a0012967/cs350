#ifndef _FILE_H_
#define _FILE_H_

struct file {
    // public
    int status;
    int offset;
    struct vnode *v;
    struct lock *file_lock; // when doing operations on the file

    // private
    int numrefs; // number of refs to the file (not the same as refs to vnode)
};

// creates an file. returns NULL on fail
struct file* f_create(int status, int offset, struct vnode *v);

// free memory used by file
void f_destroy(struct file *f);

#endif // _FILE_H_
