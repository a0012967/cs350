#ifndef _LINKEDLIST_H_
#define _LINKEDLIST_H_


/* Linked list of void pointers
 *
 * Functions:
 *     ll_create     - allocates a new linkedlist.
 *                     Returns NULL on error.
 *     ll_destroy    - dispose of the linked list.
 *     ll_empty      - return true if linked list is empty.
 *     ll_size       - return size of linked list.
 *     ll_push_front - pushes a void pointer in front of the linked list.
 *                     Returns 0 if successful, -1 otherwise.
 *     ll_push_back  - pushes a void pointer in the end of the linkst list.
 *                     Returns 0 if successful, -1 otherwise.
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

// returns a SOFT COPY of the linkedlist
struct linkedlist *ll_copy(struct linkedlist *ll);

#endif /* _LINKEDLIST_H_ */
