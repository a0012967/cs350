#ifndef _TABLE_H_
#define _TABLE_H_

// TABLE data structure. It simply is an array with one added Functionality
// It keeps track of holes

// NOTE: THIS TABLE WILL HAVE HOLES IN THE MIDDLE OF THE ARRAY
// THESE VALUES WILL BE NULL.
// BE CAREFUL WHEN ITERATING THROUGH THE TABLE
struct table;

struct table *tab_create();

// frees memory used by table
// make sure you freed memory of pointers you've inserted
void tab_destroy(struct table *t);

// returns index where ptr was stored
// returns -1 on failure and changes value of err
int tab_add(struct table *t, void *ptr, int *err);

// removes the entry at the given index
// returns 0 if successful, -1 otherwise
// sets NULL to pointer in table at the given index
int tab_remove(struct table *t, int index);

// returns the ptr at the given index
// returns NULL when ptr has been removed
void* tab_getguy(struct table *t, int index);

// DUPLICATES TABLE
// returns error code on failure
int tab_duplicate(struct table *t, struct table **new_t);

// returns number of entries including NULL in table
int tab_getsize(struct table *t);

// WARNING! DON'T USE THIS FOR NOW! USE THIS FOR THE SAKE OF DEBUGGING!
// returns number of entries in table - number of holes
int tab_getnum(struct table *t);

#endif /* _TABLE_H_ */
