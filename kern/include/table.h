#ifndef _TABLE_H_
#define _TABLE_H_

// TABLE data structure. It simply is an array with one added Functionality
// It keeps track of holes
struct table;

struct table *tab_create();
void tab_destroy(struct table *t);

//int tab_first_empty_slot(struct table *t);
//int tab_insert(struct table *t, int index, void *ptr);

// returns index where ptr was stored
int tab_add(struct table *t, void *ptr);
// removes the entry at the given index
int tab_remove(struct table *t, int index);
// returns number of entries in table - number of holes
int tab_getnum(struct table *t);
// returns number of entries in table
int tab_getsize(struct table *t);

#endif /* _TABLE_H_ */
