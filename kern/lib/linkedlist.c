#include <types.h>
#include <lib.h>
#include <linkedlist.h>


/*
 * NODE IMPLEMENTATIONS
 *
 * Node used for linked list
 * Contains: 
 *     void pointer for data.
 *     a node pointer to the next node>
 * Functions
 *     n_create  - allocates a new node.
 *                 Returns NULL on error>
 *     n_destroy - dispose of the node.
 */

struct node {
    void *data;
    struct node *next;
};

struct node* n_create(void *data, struct node *next) {
    struct node *n = kmalloc(sizeof(struct node));
    if (n == NULL)
        return NULL;
    n->data = data;
    n->next = next;
    return n;
}

void n_destroy(struct node *n) {
    assert(n != NULL);
    kfree(n);
}


/*
 * LINKED LIST IMPLEMENTATIONS
 */
struct linkedlist {
    struct node *head;
    struct node *tail;
    int size;
};

struct linkedlist* ll_create() {
    struct linkedlist *ll = kmalloc(sizeof(struct linkedlist));
    if (ll==NULL) return NULL;
    ll->head = NULL;
    ll->tail = NULL;
    ll->size = 0;
    return ll;
}

void ll_destroy(struct linkedlist *ll) {
    assert(ll != NULL);
    // destroy all elements
    if (!ll_empty(ll)) {
        // remove this assert later
        assert(ll->head != NULL);
        struct node *curr = ll->head;
        while (curr != NULL) {
            struct node *tmp = curr;
            curr = curr->next;
            kfree(tmp);
        }
    }
    kfree(ll);
}

int ll_empty(struct linkedlist *ll) {
    assert(ll != NULL);
    return ll->size == 0;
}

int ll_size(struct linkedlist *ll) {
    assert(ll != NULL);
    return ll->size;
}

// returns 0 if successful, -1 otherwise
int ll_push_front(struct linkedlist *ll, void *ptr) {
    assert(ll != NULL);
    struct node *n = n_create(ptr, NULL);

    if (n == NULL) // n_create failed
        return -1;

    if (ll->head == NULL) {
        ll->head = n;
        ll->tail = n;
    } else {
        // update the head
        n->next = ll->head;
        ll->head = n;
    }

    ll->size++;

    return 0;
}

// returns 0 if successful, -1 otherwise
int ll_push_back(struct linkedlist *ll, void *ptr) {
    assert(ll != NULL);
    struct node *n = n_create(ptr, NULL);

    if (n == NULL) // n_create failed
        return -1;

    if (ll->head == NULL) {
        ll->head = n;
        ll->tail = n;
    } else {
        ll->tail->next = n;
        ll->tail = n;
    }

    ll->size++;

    return 0;
}

// removes head and updates it
// asserts that linked list is not empty
void* ll_pop_front(struct linkedlist *ll) {
    assert(ll != NULL);
    assert(!ll_empty(ll));

    struct node *n = ll->head;
    void *ret;

    ll->head = n->next;

    // if only element update tail as well
    if (ll->head == NULL) {
        ll->tail = NULL;
    }

    ret = n->data;
    n_destroy(n);
    ll->size--;

    return ret;
}


// removes tail and updates it
// asserts that linked list is not empty
void* ll_pop_back(struct linkedlist *ll) {
    assert(ll != NULL);
    assert(!ll_empty(ll));

    struct node *curr = ll->head;
    void *ret;

    // if only element
    if (ll->head == ll->tail) {
        ret = ll->tail->data;
        n_destroy(ll->tail);
        ll->head = NULL;
        ll->tail = NULL;
        ll->size = 0;
        return ret;
    }

    // traverse through the linked list to find the prev node
    while (curr->next != ll->tail) {
        curr = curr->next;
    }

    struct node *n = curr->next;
    curr->next = NULL;
    ll->tail = curr;
    ret = n->data;
    n_destroy(n);
    ll->size--;

    return ret;
}

void *ll_front(struct linkedlist *ll) {
    assert(ll != NULL);
    if (ll_empty(ll)) 
        return NULL;
    return ll->head->data;
}

void *ll_back(struct linkedlist *ll) {
    assert(ll != NULL);
    if (ll_empty(ll)) 
        return NULL;
    return ll->tail->data;
}

// for debugging purposes only
void ll_print(struct linkedlist *ll) {
    assert(ll != NULL);
    struct node *n = ll->head;
    while (n != NULL) {
        kprintf("value: %d\n", *((int*)(n->data)));
        n = n->next;
    }
    if (ll->head != NULL)
        kprintf("head: %d\n", *((int*)(ll->head->data)));
    else
        kprintf("head: NULL\n");
    if (ll->tail != NULL)
        kprintf("tail: %d\n", *((int*)(ll->tail->data)));
    else
        kprintf("tail: NULL\n");
    kprintf("size: %d\n", ll->size);
}
