#ifndef _TABLE_H_
#define _TABLE_H_

// TABLE data structure. It simply is an array with one added Functionality
// It keeps track of holes
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
int tab_remove(struct table *t, int index);
// returns the ptr at the given index
// returns NULL when ptr has been removed
void* tab_getguy(struct table *t, int index);
// returns number of entries in table - number of holes
int tab_getnum(struct table *t);
// returns number of entries in table
int tab_getsize(struct table *t);

#endif /* _TABLE_H_ */
