#ifndef _SYSTEMFILETABLE_H_
#define _SYSTEMFILETABLE_H_

struct file;

// bootstrap the system file table
void systemfiletable_bootstrap();

// returns ERROR CODE on failure
// only insert a newly created file
int systemft_insert(struct file *f);

// returns ERROR CODE on failure
// decrements reference count of file
// if reference count is 0 removes the file and calls f_destroy
int systemft_remove(struct file *f);

// returns ERROR CODE on failure
// increments the reference count of file
int systemft_update(struct file *f);


// ONLY USED FOR DEBUGGING PURPOSES!!!

// returns the internal member count
int systemft_getcount();

// iterates to count number of elements and returns it
int systemft_iter_ct();

#endif // _SYSTEMFILETABLE_H_
