#ifndef _LINKEDLIST_H_
#define _LINKEDLIST_H_


/*
 * Node used for linked list
 * Contains: 
 *     void pointer for data.
 *     a node pointer to the next node>
 * Functions
 *     n_create  - allocates a new node.
 *                 Returns NULL on error>
 *     n_destroy - dispose of the node.
 */

struct node; // opaque

struct node *n_create(void *data, struct node *next);
void         n_destroy(struct node *n);


/* Linked list of void pointers
 *
 * Functions:
 *     ll_create     - allocates a new linkedlist.
 *                     Returns NULL on error.
 *     ll_destroy    - dispose of the linked list.
 *     ll_empty      - return true if linked list is empty.
 *     ll_size       - return size of linked list.
 *     ll_push_front - pushes a void pointer in front of the linked list.
 *                     Returns 1 if successful, 0 otherwise.
 *     ll_push_back  - pushes a void pointer in the end of the linkst list.
 *                     Returns 1 if successful, 0 otherwise.
 *     ll_pop_front  - removes and returns a pointer from the front of the linked list.
 *                     Asserts that linked list is not empty.
 *     ll_pop_back   - removes and returns a pointer from the back of the linked list.
 *                     Asserts that linked list is not empty.
 *     ll_front      - returns the pointer from the front of the linked list.
 *     ll_back       - returns the pointer from the back of the linked list.
 *
 */

struct linkedlist; //opaque

struct linkedlist *ll_create();
void               ll_destroy(struct linkedlist *ll);
int                ll_empty(struct linkedlist *ll);
int                ll_size(struct linkedlist *ll);
int                ll_push_front(struct linkedlist *ll, void *ptr);
int                ll_push_back(struct linkedlist *ll, void *ptr);
void              *ll_pop_front(struct linkedlist *ll);
void              *ll_pop_back(struct linkedlist *ll);
void              *ll_front(struct linkedlist *ll);
void              *ll_back(struct linkedlist *ll);

// for debugging purposes only. NEVER USE THIS!
void ll_print(struct linkedlist *ll);

#endif /* _LINKEDLIST_H_ */
