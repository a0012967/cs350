#ifndef _FILE_H_
#define _FILE_H_

struct file {
    // PUBLIC
    int status;
    int offset;
    struct vnode *v;
    struct lock *file_lock; // when doing operations on the file

    // PRIVATE: don't touch this!
    // INTERNALS to system-wide filetable
    int count;
    struct file *prev;
    struct file *next;
};

#endif // _FILE_H_

// creates an file. returns NULL on fail
struct file* f_create(int status, int offset, struct vnode *v);
// free memory used by file
void f_destroy(struct file *f);
